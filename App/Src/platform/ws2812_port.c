#include "platform/ws2812_port.h"

#include <string.h>

#ifndef WS2812_HOST_TEST
#include "tim.h"
#endif

enum {
    WS2812_T0H_NS = 350u,
    WS2812_T1H_NS = 700u
};

bool ws2812_compare_ticks(uint32_t timer_clock_hz, uint32_t period_ticks,
                          uint32_t *duty_0, uint32_t *duty_1)
{
    if (timer_clock_hz == 0u || period_ticks == 0u || duty_0 == NULL ||
        duty_1 == NULL) {
        return false;
    }

    const uint32_t calculated_duty_0 = (uint32_t)(
        ((uint64_t)timer_clock_hz * WS2812_T0H_NS + 500000000u) /
        1000000000u);
    const uint32_t calculated_duty_1 = (uint32_t)(
        ((uint64_t)timer_clock_hz * WS2812_T1H_NS + 500000000u) /
        1000000000u);
    if (calculated_duty_0 == 0u ||
        calculated_duty_0 >= calculated_duty_1 ||
        calculated_duty_1 >= period_ticks) {
        return false;
    }

    *duty_0 = calculated_duty_0;
    *duty_1 = calculated_duty_1;
    return true;
}

static size_t encode_component_pair(uint8_t first, uint8_t second,
                                    uint32_t duty_0, uint32_t duty_1,
                                    uint32_t *interleaved, size_t offset)
{
    for (uint8_t bit = 0x80u; bit != 0u; bit >>= 1u) {
        interleaved[offset++] = (first & bit) != 0u ? duty_1 : duty_0;
        interleaved[offset++] = (second & bit) != 0u ? duty_1 : duty_0;
    }
    return offset;
}

static bool pair_encoding_slots(const LedRgb *first, const LedRgb *second,
                                size_t pixel_count, uint32_t duty_0,
                                uint32_t duty_1, size_t reset_slots,
                                const uint32_t *interleaved,
                                size_t capacity_words, size_t *slots_out)
{
    if (first == NULL || second == NULL || interleaved == NULL ||
        slots_out == NULL || pixel_count == 0u ||
        reset_slots < WS2812_MIN_RESET_SLOTS || duty_0 >= duty_1 ||
        pixel_count > (SIZE_MAX - reset_slots) / 24u) {
        return false;
    }

    const size_t slots = pixel_count * 24u + reset_slots;
    if (slots > SIZE_MAX / 2u || capacity_words < slots * 2u) {
        return false;
    }
    *slots_out = slots;
    return true;
}

size_t ws2812_encode_pair(const LedRgb *first, const LedRgb *second,
                          size_t pixel_count, uint32_t duty_0,
                          uint32_t duty_1, size_t reset_slots,
                          uint32_t *interleaved, size_t capacity_words)
{
    size_t slots = 0u;
    if (!pair_encoding_slots(first, second, pixel_count, duty_0, duty_1,
                             reset_slots, interleaved, capacity_words,
                             &slots)) {
        return 0u;
    }

    size_t offset = 0u;
    for (size_t pixel = 0u; pixel < pixel_count; ++pixel) {
        offset = encode_component_pair(first[pixel].g, second[pixel].g,
                                       duty_0, duty_1, interleaved, offset);
        offset = encode_component_pair(first[pixel].r, second[pixel].r,
                                       duty_0, duty_1, interleaved, offset);
        offset = encode_component_pair(first[pixel].b, second[pixel].b,
                                       duty_0, duty_1, interleaved, offset);
    }
    memset(&interleaved[offset], 0, reset_slots * 2u * sizeof *interleaved);
    return slots;
}

bool ws2812_can_submit(uint32_t now_ms, uint32_t last_submit_ms, bool idle)
{
    return idle && (uint32_t)(now_ms - last_submit_ms) >=
                       WS2812_RATE_LIMIT_MS;
}

void ws2812_transport_init(Ws2812Transport *transport)
{
    if (transport == NULL) {
        return;
    }

    transport->state = WS2812_TRANSPORT_IDLE;
    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->last_submit_ms = UINT32_MAX - (WS2812_RATE_LIMIT_MS - 1u);
    transport->error_count = 0u;
    transport->busy_drop_count = 0u;
}

static bool transport_pairs_complete(const Ws2812Transport *transport)
{
    return transport->pair_complete[0] && transport->pair_complete[1];
}

static void transport_fail(Ws2812Transport *transport, Ws2812PairStopFn stop,
                           void *ctx, bool stop_pair_0, bool stop_pair_1)
{
    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->state = WS2812_TRANSPORT_IDLE;
    transport->error_count++;

    if (stop_pair_0) {
        stop(0u, ctx);
    }
    if (stop_pair_1) {
        stop(1u, ctx);
    }
}

bool ws2812_transport_submit(Ws2812Transport *transport, uint32_t now_ms,
                             const Ws2812Frame *frame, uint32_t duty_0,
                             uint32_t duty_1, size_t reset_slots,
                             uint32_t *pair_0_words,
                             size_t pair_0_capacity_words,
                             uint32_t *pair_1_words,
                             size_t pair_1_capacity_words,
                             Ws2812PairStartFn start,
                             Ws2812PairStopFn stop,
                             Ws2812CriticalEnterFn critical_enter,
                             Ws2812CriticalExitFn critical_exit, void *ctx)
{
    if (transport == NULL || frame == NULL || start == NULL || stop == NULL ||
        critical_enter == NULL || critical_exit == NULL) {
        return false;
    }

    size_t validated_slots = 0u;
    if (!pair_encoding_slots(
            frame->groups[0], frame->groups[1], WS2812_GROUP_LENGTHS[0],
            duty_0, duty_1, reset_slots, pair_0_words,
            pair_0_capacity_words, &validated_slots) ||
        !pair_encoding_slots(
            frame->groups[2], frame->groups[3], WS2812_GROUP_LENGTHS[2],
            duty_0, duty_1, reset_slots, pair_1_words,
            pair_1_capacity_words, &validated_slots)) {
        return false;
    }
    if (transport->state != WS2812_TRANSPORT_IDLE) {
        transport->busy_drop_count++;
        return false;
    }
    if (!ws2812_can_submit(now_ms, transport->last_submit_ms, true)) {
        return false;
    }

    const size_t pair_0_slots = ws2812_encode_pair(
        frame->groups[0], frame->groups[1], WS2812_GROUP_LENGTHS[0], duty_0,
        duty_1, reset_slots, pair_0_words, pair_0_capacity_words);
    const size_t pair_1_slots = ws2812_encode_pair(
        frame->groups[2], frame->groups[3], WS2812_GROUP_LENGTHS[2], duty_0,
        duty_1, reset_slots, pair_1_words, pair_1_capacity_words);
    if (pair_0_slots == 0u || pair_1_slots == 0u) {
        return false;
    }

    const uintptr_t saved_state = critical_enter(ctx);
    if (transport->state != WS2812_TRANSPORT_IDLE) {
        transport->busy_drop_count++;
        critical_exit(saved_state, ctx);
        return false;
    }
    if (!ws2812_can_submit(now_ms, transport->last_submit_ms, true)) {
        critical_exit(saved_state, ctx);
        return false;
    }

    transport->pair_complete[0] = false;
    transport->pair_complete[1] = false;
    transport->state = WS2812_TRANSPORT_STARTING;
    const bool pair_0_started = start(0u, pair_0_words, pair_0_slots, ctx);
    if (transport->state != WS2812_TRANSPORT_STARTING) {
        critical_exit(saved_state, ctx);
        return false;
    }
    if (!pair_0_started) {
        transport_fail(transport, stop, ctx, false, false);
        critical_exit(saved_state, ctx);
        return false;
    }

    const bool pair_1_started = start(1u, pair_1_words, pair_1_slots, ctx);
    if (pair_1_started && transport->state == WS2812_TRANSPORT_IDLE &&
        transport_pairs_complete(transport)) {
        transport->last_submit_ms = now_ms;
        critical_exit(saved_state, ctx);
        return true;
    }
    if (transport->state != WS2812_TRANSPORT_STARTING) {
        critical_exit(saved_state, ctx);
        return false;
    }
    if (!pair_1_started) {
        transport_fail(transport, stop, ctx, true, false);
        critical_exit(saved_state, ctx);
        return false;
    }

    transport->last_submit_ms = now_ms;
    transport->state = transport_pairs_complete(transport)
                           ? WS2812_TRANSPORT_IDLE
                           : WS2812_TRANSPORT_ACTIVE;
    critical_exit(saved_state, ctx);
    return true;
}

void ws2812_transport_complete(Ws2812Transport *transport, unsigned pair)
{
    if (transport == NULL || transport->state == WS2812_TRANSPORT_IDLE ||
        pair >= WS2812_PAIR_COUNT) {
        return;
    }

    transport->pair_complete[pair] = true;
    if (transport_pairs_complete(transport)) {
        transport->state = WS2812_TRANSPORT_IDLE;
    }
}

void ws2812_transport_error(Ws2812Transport *transport, unsigned pair,
                            Ws2812PairStopFn stop, void *ctx)
{
    if (transport == NULL || transport->state == WS2812_TRANSPORT_IDLE ||
        pair >= WS2812_PAIR_COUNT || stop == NULL) {
        return;
    }

    transport_fail(transport, stop, ctx, true, true);
}

uint32_t ws2812_transport_errors(const Ws2812Transport *transport)
{
    return transport != NULL ? transport->error_count : 0u;
}

uint32_t ws2812_transport_busy_drops(const Ws2812Transport *transport)
{
    return transport != NULL ? transport->busy_drop_count : 0u;
}

#ifndef WS2812_HOST_TEST

enum {
    WS2812_CACHE_LINE_BYTES = 32u,
    WS2812_DMA_MAX_WORDS = 0xffffu,
    WS2812_PAIR_0_PIXELS = 4u,
    WS2812_PAIR_1_PIXELS = 8u,
    WS2812_PAIR_0_SLOTS = WS2812_PAIR_0_PIXELS * 24u + WS2812_MIN_RESET_SLOTS,
    WS2812_PAIR_1_SLOTS = WS2812_PAIR_1_PIXELS * 24u + WS2812_MIN_RESET_SLOTS,
    WS2812_PAIR_0_WORDS = WS2812_PAIR_0_SLOTS * 2u,
    WS2812_PAIR_1_WORDS = WS2812_PAIR_1_SLOTS * 2u
};

static Ws2812Transport ws2812_transport;
static bool ws2812_initialized;
static uint32_t ws2812_duty_0;
static uint32_t ws2812_duty_1;
static uint32_t ws2812_pair_0_words[WS2812_PAIR_0_WORDS]
    __attribute__((aligned(WS2812_CACHE_LINE_BYTES)));
static uint32_t ws2812_pair_1_words[WS2812_PAIR_1_WORDS]
    __attribute__((aligned(WS2812_CACHE_LINE_BYTES)));

static TIM_HandleTypeDef *ws2812_pair_timer(unsigned pair)
{
    if (pair == 0u) {
        return &htim2;
    }
    if (pair == 1u) {
        return &htim3;
    }
    return NULL;
}

static void ws2812_pair_pins_low(unsigned pair)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    if (pair == 0u) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        gpio.Pin = GPIO_PIN_0;
        HAL_GPIO_Init(GPIOA, &gpio);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        gpio.Pin = GPIO_PIN_3;
        HAL_GPIO_Init(GPIOB, &gpio);
    } else if (pair == 1u) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4 | GPIO_PIN_5, GPIO_PIN_RESET);
        gpio.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
}

static void ws2812_all_pins_low(void)
{
    ws2812_pair_pins_low(0u);
    ws2812_pair_pins_low(1u);
}

static void ws2812_pair_stop(unsigned pair, void *ctx)
{
    (void)ctx;
    TIM_HandleTypeDef *timer = ws2812_pair_timer(pair);
    if (timer == NULL) {
        return;
    }

    (void)HAL_TIM_PWM_Stop(timer, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Stop(timer, TIM_CHANNEL_2);
    (void)HAL_TIM_DMABurst_WriteStop(timer, TIM_DMA_UPDATE);
    __HAL_TIM_SET_COMPARE(timer, TIM_CHANNEL_1, 0u);
    __HAL_TIM_SET_COMPARE(timer, TIM_CHANNEL_2, 0u);
    ws2812_pair_pins_low(pair);
}

static void ws2812_restore_pair_alternate_function(unsigned pair)
{
    TIM_HandleTypeDef *timer = ws2812_pair_timer(pair);
    if (timer != NULL) {
        HAL_TIM_MspPostInit(timer);
    }
}

static bool ws2812_pair_start(unsigned pair, const uint32_t *words,
                              size_t slots, void *ctx)
{
    (void)ctx;
    TIM_HandleTypeDef *timer = ws2812_pair_timer(pair);
    if (timer == NULL || words == NULL || slots == 0u ||
        slots > WS2812_DMA_MAX_WORDS / 2u) {
        return false;
    }

    const uint32_t data_length = (uint32_t)(slots * 2u);
    const size_t encoded_bytes = (size_t)data_length * sizeof *words;
    const size_t clean_bytes =
        (encoded_bytes + WS2812_CACHE_LINE_BYTES - 1u) &
        ~(size_t)(WS2812_CACHE_LINE_BYTES - 1u);

    ws2812_restore_pair_alternate_function(pair);
    SCB_CleanDCache_by_Addr((uint32_t *)words, (int32_t)clean_bytes);
    __HAL_TIM_SET_COUNTER(timer, 0u);
    __HAL_TIM_SET_COMPARE(timer, TIM_CHANNEL_1, 0u);
    __HAL_TIM_SET_COMPARE(timer, TIM_CHANNEL_2, 0u);

    if (HAL_TIM_DMABurst_MultiWriteStart(
            timer, TIM_DMABASE_CCR1, TIM_DMA_UPDATE, words,
            TIM_DMABURSTLENGTH_2TRANSFERS,
            (uint32_t)(slots * 2u)) != HAL_OK) {
        ws2812_pair_stop(pair, NULL);
        ws2812_all_pins_low();
        return false;
    }
    if (HAL_TIM_PWM_Start(timer, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_Start(timer, TIM_CHANNEL_2) != HAL_OK) {
        ws2812_pair_stop(pair, NULL);
        ws2812_all_pins_low();
        return false;
    }
    return true;
}

static uintptr_t ws2812_critical_enter(void *ctx)
{
    (void)ctx;
    const uint32_t saved_primask = __get_PRIMASK();
    __disable_irq();
    return (uintptr_t)saved_primask;
}

static void ws2812_critical_exit(uintptr_t saved_state, void *ctx)
{
    (void)ctx;
    __set_PRIMASK((uint32_t)saved_state);
}

static void ws2812_noop_stop(unsigned pair, void *ctx)
{
    (void)pair;
    (void)ctx;
}

void ws2812_port_init(void)
{
    ws2812_initialized = false;
    ws2812_transport_init(&ws2812_transport);
    ws2812_all_pins_low();
    MX_TIM2_Init();
    MX_TIM3_Init();

    const uint32_t period_ticks = htim2.Init.Period + 1u;
    if (htim3.Init.Period != htim2.Init.Period ||
        !ws2812_compare_ticks(ws2812_timer_clock_hz(), period_ticks,
                              &ws2812_duty_0, &ws2812_duty_1)) {
        ws2812_all_pins_low();
        return;
    }
    ws2812_initialized = true;
}

bool ws2812_port_submit(uint32_t now_ms, const Ws2812Frame *frame)
{
    if (!ws2812_initialized) {
        return false;
    }

    return ws2812_transport_submit(
        &ws2812_transport, now_ms, frame, ws2812_duty_0, ws2812_duty_1,
        WS2812_MIN_RESET_SLOTS, ws2812_pair_0_words,
        WS2812_PAIR_0_WORDS, ws2812_pair_1_words, WS2812_PAIR_1_WORDS,
        ws2812_pair_start, ws2812_pair_stop, ws2812_critical_enter,
        ws2812_critical_exit, NULL);
}

uint32_t ws2812_port_busy_drops(void)
{
    return ws2812_transport_busy_drops(&ws2812_transport);
}

void ws2812_port_pair_complete(unsigned pair)
{
    if (pair >= WS2812_PAIR_COUNT) {
        return;
    }
    ws2812_pair_stop(pair, NULL);
    ws2812_transport_complete(&ws2812_transport, pair);
}

void ws2812_port_pair_error(unsigned pair)
{
    if (pair >= WS2812_PAIR_COUNT ||
        ws2812_transport.state == WS2812_TRANSPORT_IDLE) {
        return;
    }

    ws2812_pair_stop(0u, NULL);
    ws2812_pair_stop(1u, NULL);
    ws2812_transport_error(&ws2812_transport, pair, ws2812_noop_stop, NULL);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim2) {
        ws2812_port_pair_complete(0u);
    } else if (htim == &htim3) {
        ws2812_port_pair_complete(1u);
    }
}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim2) {
        ws2812_port_pair_error(0u);
    } else if (htim == &htim3) {
        ws2812_port_pair_error(1u);
    }
}

#endif
