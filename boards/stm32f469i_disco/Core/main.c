#include "stm32f4xx_hal.h"
#include "stm32469i_discovery.h"

#include "clock.h"
#include "display_touch.h"
#include "lamp_f469.h"
#include "msp_uart_f469.h"
#include "ws2812_f469.h"
#include "app_config.h"
#include "diagnostics.h"
#include "lighting/lighting_service.h"
#include "msp/msp_client.h"
#include "ui/f469_ui.h"
#include "ui/low_battery_policy.h"
#include "vehicle_state.h"
#include "vehicle_state_source.h"

static MspClient client;
static LightingService lights;
static LowBatteryPolicy battery;
static VehicleStateSource presentation;
static DiagnosticsSnapshot diagnostics;
static IWDG_HandleTypeDef watchdog;
static uint32_t last_ui_ms, last_lighting_ms, last_diag_ms;
static uint32_t frames, last_button_ms;
static uint32_t last_cycle, max_loop_us;
static volatile uint32_t ui_build_us, lvgl_render_us;
static uint8_t watchdog_progress;
static bool last_button_pressed;

static bool write_msp(const uint8_t *data, size_t count, void *ctx)
{
    (void)ctx;
    return f469_msp_uart_write(data, count);
}
static void on_frame(const MspFrame *frame, uint32_t now_ms, void *ctx)
{
    (void)ctx;
    if (vehicle_state_on_msp(frame, now_ms))
        vehicle_state_source_note_real(&presentation, now_ms);
}
static void apply_lamps(uint16_t front, uint16_t roof, void *ctx)
{
    (void)ctx;
    f469_lamp_apply(front, roof);
}
static bool submit_leds(uint32_t now_ms, const Ws2812Frame *frame, void *ctx)
{
    (void)ctx;
    return f469_ws2812_submit(now_ms, frame);
}
static void fail_safe(void)
{
    f469_lamp_apply(0u, 0u);
    f469_ws2812_force_low();
    for (;;) { HAL_Delay(100u); }
}
int main(void)
{
    HAL_Init();
    diagnostics.reset_flags = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
    f469_lamp_init_off();
    f469_ws2812_force_low();
    if (!f469_clock_init()) fail_safe();
    vehicle_state_init();
    vehicle_state_source_init(&presentation);
    low_battery_policy_init(&battery, APP_BATTERY_CELL_COUNT);
    lighting_service_init(&lights);
    msp_client_init(&client, write_msp, on_frame, NULL);
    if (!f469_msp_uart_init()) fail_safe();
    if (!f469_ws2812_init()) fail_safe();
    if (!f469_display_touch_init()) fail_safe();
    if (!f469_ui_init()) fail_safe();
    diagnostics.touch_available = f469_touch_available();
    diagnostics.state = HEALTH_OK;
    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);
    watchdog.Instance = IWDG;
    watchdog.Init.Prescaler = IWDG_PRESCALER_64;
    watchdog.Init.Reload = 2500u;
    if (HAL_IWDG_Init(&watchdog) != HAL_OK) fail_safe();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    last_cycle = DWT->CYCCNT;
    const uint32_t start = HAL_GetTick();
    last_ui_ms = last_lighting_ms = last_diag_ms = start;
    last_button_ms = start - 200u;

    for (;;) {
        const uint32_t now_ms = HAL_GetTick();
        uint8_t byte;
        while (f469_msp_uart_read(&byte)) msp_client_rx_byte(&client, byte, now_ms);
        msp_client_tick(&client, now_ms);
        watchdog_progress |= DIAG_PROGRESS_MSP;
        vehicle_state_tick(now_ms);
        const VehicleState *real = vehicle_state_get();
        vehicle_state_source_tick(&presentation, now_ms, APP_BATTERY_CELL_COUNT, real);
        const bool demo = vehicle_state_source_is_demo(&presentation);
        const bool low_battery = low_battery_policy_update_from_vehicle(&battery, demo, real);

        const bool pressed = BSP_PB_GetState(BUTTON_USER) != 0u;
        if (!diagnostics.touch_available && pressed && !last_button_pressed &&
            (uint32_t)(now_ms - last_button_ms) >= 200u) {
            f469_ui_button(false);
            last_button_ms = now_ms;
        }
        last_button_pressed = pressed;

        if ((uint32_t)(now_ms - last_lighting_ms) >= 34u) {
            diagnostics.led_current_ma = lighting_service_tick(
                &lights, now_ms, real, low_battery,
                diagnostics.state == HEALTH_FAULT,
                apply_lamps, submit_leds, NULL);
            last_lighting_ms = now_ms;
            watchdog_progress |= DIAG_PROGRESS_LED;
        }
        if ((uint32_t)(now_ms - last_ui_ms) >= 33u) {
            const uint32_t ui_start = DWT->CYCCNT;
            f469_ui_tick(now_ms, vehicle_state_source_get(&presentation),
                          &diagnostics, low_battery, demo);
            const uint32_t render_start = DWT->CYCCNT;
            ui_build_us = (uint32_t)(((uint64_t)(render_start - ui_start) * 1000000u) /
                                      SystemCoreClock);
            f469_display_touch_tick(now_ms);
            lvgl_render_us = (uint32_t)(((uint64_t)(DWT->CYCCNT - render_start) * 1000000u) /
                                         SystemCoreClock);
            ++frames;
            last_ui_ms = now_ms;
            watchdog_progress |= DIAG_PROGRESS_UI;
        }
        const uint32_t cycle = DWT->CYCCNT;
        const uint32_t loop_us = (uint32_t)(((uint64_t)(cycle - last_cycle) * 1000000u) /
                                            SystemCoreClock);
        last_cycle = cycle;
        if (loop_us > max_loop_us) max_loop_us = loop_us;
        if ((uint32_t)(now_ms - last_diag_ms) >= 1000u) {
            f469_touch_health_check();
            diagnostics.touch_available = f469_touch_available();
            diagnostics.fps = (uint16_t)frames;
            frames = 0u;
            if (diagnostics.fps < 20u) ++diagnostics.frame_misses;
            else diagnostics.frame_misses = 0u;
            diagnostics.max_loop_us = max_loop_us;
            max_loop_us = 0u;
            diagnostics.msp_timeouts = client.timeouts;
            diagnostics.uart_overruns = f469_msp_uart_overruns();
            diagnostics.ws2812_busy_drops = f469_ws2812_busy_drops();
            diagnostics.msp_age_ms = client.last_valid_ms == 0u ? now_ms :
                                      (uint32_t)(now_ms - client.last_valid_ms);
            diagnostics.state = f469_ws2812_errors() ? HEALTH_FAULT :
                                diagnostics.uart_overruns ||
                                diagnostics.frame_misses >= 3u ?
                                HEALTH_DEGRADED : HEALTH_OK;
            last_diag_ms = now_ms;
        }
        if (watchdog_progress == DIAG_PROGRESS_ALL) {
            (void)HAL_IWDG_Refresh(&watchdog);
            watchdog_progress = 0u;
        }
        HAL_Delay(1u);
    }
}
