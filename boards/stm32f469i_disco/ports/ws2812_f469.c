#include "ws2812_f469.h"
#include "stm32f4xx_hal.h"

enum { RESET_SLOTS = 64, A_PIXELS = 8, B_PIXELS = 16,
       SLOTS = RESET_SLOTS * 2 + B_PIXELS * 24,
       DMA_WORDS = SLOTS * 2 };
static TIM_HandleTypeDef timer;
static DMA_HandleTypeDef dma;
static uint32_t interleaved[DMA_WORDS];
static volatile bool busy;
static uint32_t last_submit, busy_drops, errors, duty_zero, duty_one;
static volatile uint32_t start_failures, dma_failures, completions;
static volatile uint32_t last_dma_error_code, last_dma_state;

static void pin_low(uint16_t pin)
{
    HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_RESET);
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
}
void f469_ws2812_force_low(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    pin_low(GPIO_PIN_4);
    pin_low(GPIO_PIN_5);
}
static void pin_timer(uint16_t pin)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &gpio);
}
static bool init_dma(DMA_HandleTypeDef *dma, DMA_Stream_TypeDef *stream)
{
    dma->Instance = stream;
    dma->Init.Channel = DMA_CHANNEL_5;
    dma->Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    dma->Init.Mode = DMA_NORMAL;
    dma->Init.Priority = DMA_PRIORITY_HIGH;
    dma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    return HAL_DMA_Init(dma) == HAL_OK;
}
bool f469_ws2812_init(void)
{
    f469_ws2812_force_low();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    if (!init_dma(&dma, DMA1_Stream2)) return false;
    __HAL_LINKDMA(&timer, hdma[TIM_DMA_ID_UPDATE], dma);
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    const uint32_t timer_hz = HAL_RCC_GetPCLK1Freq() * 2u;
    const uint32_t period = (timer_hz + 400000u) / 800000u;
    if (period < 3u || period > 65536u) return false;
    duty_zero = (uint32_t)(((uint64_t)timer_hz * 350u + 500000000u) / 1000000000u);
    duty_one = (uint32_t)(((uint64_t)timer_hz * 700u + 500000000u) / 1000000000u);
    if (duty_zero == 0u || duty_zero >= duty_one || duty_one >= period) return false;
    timer.Instance = TIM3;
    timer.Init.Prescaler = 0;
    timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    timer.Init.Period = period - 1u;
    timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&timer) != HAL_OK) return false;
    TIM_OC_InitTypeDef channel = {0};
    channel.OCMode = TIM_OCMODE_PWM1;
    channel.Pulse = 0;
    channel.OCPolarity = TIM_OCPOLARITY_HIGH;
    channel.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&timer, &channel, TIM_CHANNEL_1) != HAL_OK ||
        HAL_TIM_PWM_ConfigChannel(&timer, &channel, TIM_CHANNEL_2) != HAL_OK) return false;
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE;
    last_submit = UINT32_MAX - 15u;
    return true;
}
static void encode_byte(uint8_t value, unsigned offset, unsigned channel)
{
    for (unsigned bit = 0; bit < 8u; ++bit)
        interleaved[(offset + bit) * 2u + channel] =
            (value & (0x80u >> bit)) ? duty_one : duty_zero;
}
static void encode_frame(const Ws2812Frame *frame)
{
    for (unsigned i = 0; i < DMA_WORDS; ++i) interleaved[i] = 0u;
    for (unsigned chain = 0; chain < 2u; ++chain) {
        const unsigned count = chain == 0u ? A_PIXELS : B_PIXELS;
        for (unsigned i = 0; i < count; ++i) {
            const LedRgb pixel = f469_ws2812_chain_pixel(frame, chain, i);
            const unsigned slot = RESET_SLOTS + i * 24u;
            encode_byte(pixel.g, slot, chain);
            encode_byte(pixel.r, slot + 8u, chain);
            encode_byte(pixel.b, slot + 16u, chain);
        }
    }
}
static void stop_output(void)
{
    __HAL_TIM_DISABLE(&timer);
    TIM3->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E);
    __HAL_TIM_DISABLE_DMA(&timer, TIM_DMA_UPDATE);
    timer.DMABurstState = HAL_DMA_BURST_STATE_READY;
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_1, 0u);
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_2, 0u);
    f469_ws2812_force_low();
    busy = false;
}
bool f469_ws2812_submit(uint32_t now_ms, const Ws2812Frame *frame)
{
    if (frame == NULL) return false;
    if (busy) { ++busy_drops; return false; }
    if ((uint32_t)(now_ms - last_submit) < 16u) return false;
    encode_frame(frame);
    pin_timer(GPIO_PIN_4);
    pin_timer(GPIO_PIN_5);
    TIM3->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E);
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_1, 0u);
    __HAL_TIM_SET_COMPARE(&timer, TIM_CHANNEL_2, 0u);
    __HAL_TIM_SET_COUNTER(&timer, 0u);
    busy = true;
    if (HAL_TIM_DMABurst_MultiWriteStart(&timer, TIM_DMABASE_CCR1, TIM_DMA_UPDATE,
                                        interleaved, TIM_DMABURSTLENGTH_2TRANSFERS,
                                        DMA_WORDS) != HAL_OK) {
        stop_output();
        ++start_failures;
        last_dma_error_code = dma.ErrorCode;
        last_dma_state = dma.State;
        ++errors;
        return false;
    }
    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E;
    __HAL_TIM_ENABLE(&timer);
    last_submit = now_ms;
    return true;
}
uint32_t f469_ws2812_busy_drops(void) { return busy_drops; }
uint32_t f469_ws2812_errors(void) { return errors; }
void DMA1_Stream2_IRQHandler(void) { HAL_DMA_IRQHandler(&dma); }
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *handle)
{
    if (handle != &timer) return;
    ++completions;
    stop_output();
}
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *handle)
{
    if (handle != &timer) return;
    ++dma_failures;
    last_dma_error_code = dma.ErrorCode;
    last_dma_state = dma.State;
    if (dma.State == HAL_DMA_STATE_BUSY) (void)HAL_DMA_Abort(&dma);
    stop_output();
    ++errors;
}
