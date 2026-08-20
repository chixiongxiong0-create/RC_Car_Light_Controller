#include "app.h"

#include "diagnostics.h"
#include "app_config.h"
#include "input_manager.h"
#include "lighting/lighting_service.h"
#include "msp/msp_client.h"
#include "platform/lighting_output_port.h"
#include "platform/msp_uart.h"
#include "platform/button_input.h"
#include "platform/ws2812_port.h"
#include "platform/touch_probe.h"
#include "ui/low_battery_policy.h"
#ifdef BSP_CONFIG_SEEDSTUDIO
#include "platform/lvgl_port.h"
#include "ui/ui_app.h"
#endif
#include "usart.h"
#include "vehicle_state.h"
#include "vehicle_state_source.h"
#include "wio_lite_ai.h"

static MspClient client;
static LightingService lighting_service;
static LowBatteryPolicy battery_policy;
static VehicleStateSource vehicle_source;
#ifdef BSP_CONFIG_SEEDSTUDIO
static bool ui_ready;
#endif
static uint32_t frame_misses;
static uint32_t last_diag_ms;
static uint32_t last_cycle;
static uint32_t max_loop_us;
static uint32_t led_current_ma;

static bool write_msp(const uint8_t *data, size_t length, void *ctx)
{
  (void)ctx;
  return msp_uart_write(data, length);
}

static void on_msp_frame(const MspFrame *frame, uint32_t now_ms, void *ctx)
{
  (void)ctx;
  if (vehicle_state_on_msp(frame, now_ms)) {
    vehicle_state_source_note_real(&vehicle_source, now_ms);
  }
}

static int32_t read_user_button(void *ctx)
{
  (void)ctx;
  return BSP_PB_GetState(BUTTON_USER1);
}

static void set_user_button(bool pressed, uint32_t now_ms, void *ctx)
{
  (void)ctx;
  input_manager_set_button(pressed, now_ms);
}

static void apply_lighting(uint16_t front_duty, uint16_t roof_spot_duty,
                           void *ctx)
{
  (void)ctx;
  lighting_output_port_apply(front_duty, roof_spot_duty);
}

static bool submit_lighting(uint32_t now_ms, const Ws2812Frame *frame,
                            void *ctx)
{
  (void)ctx;
  return ws2812_port_submit(now_ms, frame);
}

static void lighting_tick(uint32_t now_ms, const VehicleState *real_state,
                          bool low_battery)
{
  const bool board_fault = diagnostics_get()->state == HEALTH_FAULT;
  led_current_ma = lighting_service_tick(
      &lighting_service, now_ms, real_state, low_battery, board_fault,
      apply_lighting, submit_lighting, NULL);
  diagnostics_watchdog_mark(DIAG_PROGRESS_LED);
}

void App_Init(void)
{
  lighting_output_port_init();
  lighting_service_init(&lighting_service);
  MX_USART3_UART_Init();
  msp_uart_init();
  vehicle_state_init();
  vehicle_state_source_init(&vehicle_source);
  input_manager_init();
  low_battery_policy_init(&battery_policy, APP_BATTERY_CELL_COUNT);
  msp_client_init(&client, write_msp, on_msp_frame, NULL);
  diagnostics_init();
  diagnostics_set_touch_available(touch_probe_boot() != TOUCH_NONE);
  ws2812_port_init();
  input_manager_set_touch_available(diagnostics_get()->touch_available);
#ifdef BSP_CONFIG_SEEDSTUDIO
  ui_ready = lvgl_port_init();
  if (ui_ready) {
    ui_app_init();
  }
#endif
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0u;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  last_cycle = DWT->CYCCNT;
  last_diag_ms = HAL_GetTick();
#ifdef BSP_CONFIG_SEEDSTUDIO
  diagnostics_set_init_result(ui_ready && msp_uart_rx_arm_failures() == 0u);
#else
  diagnostics_set_init_result(msp_uart_rx_arm_failures() == 0u);
#endif
}

void App_Tick(uint32_t now_ms)
{
  uint8_t byte;
  bool low_battery;
  const VehicleState *real_state;
  const VehicleState *presented;
  msp_uart_service();
  while (msp_uart_read(&byte)) {
    msp_client_rx_byte(&client, byte, now_ms);
  }
  msp_client_tick(&client, now_ms);
  diagnostics_watchdog_mark(DIAG_PROGRESS_MSP);
  vehicle_state_tick(now_ms);
  real_state = vehicle_state_get();
  vehicle_state_source_tick(&vehicle_source, now_ms, APP_BATTERY_CELL_COUNT,
                            real_state);
  low_battery = low_battery_policy_update_from_vehicle(
      &battery_policy, vehicle_state_source_is_demo(&vehicle_source),
      real_state);
  lighting_tick(now_ms, real_state, low_battery);
  presented = vehicle_state_source_get(&vehicle_source);
  button_input_poll(now_ms, read_user_button, set_user_button, NULL);
  input_manager_set_touch_available(diagnostics_get()->touch_available);
  input_manager_tick(now_ms, presented);
#ifdef BSP_CONFIG_SEEDSTUDIO
  if (ui_ready) {
    lvgl_port_tick(now_ms);
    ui_app_tick(now_ms, presented, low_battery,
                vehicle_state_source_is_demo(&vehicle_source));
    diagnostics_watchdog_mark(DIAG_PROGRESS_UI);
  }
#else
  diagnostics_watchdog_mark(DIAG_PROGRESS_UI);
#endif
  const uint32_t cycle = DWT->CYCCNT;
  const uint32_t loop_us = (uint32_t)(((uint64_t)(cycle - last_cycle) * 1000000u) /
                                      SystemCoreClock);
  last_cycle = cycle;
  if (loop_us > max_loop_us) {
    max_loop_us = loop_us;
  }
  if ((uint32_t)(now_ms - last_diag_ms) >= 1000u) {
#ifdef BSP_CONFIG_SEEDSTUDIO
    const uint16_t fps = lvgl_port_fps();
#else
    const uint16_t fps = 0u;
#endif
    if (fps != 0u && fps < 20u) {
      ++frame_misses;
    } else {
      frame_misses = 0u;
    }
    const uint32_t age = client.last_valid_ms == 0u
                           ? now_ms : (uint32_t)(now_ms - client.last_valid_ms);
    diagnostics_set_runtime(fps, max_loop_us, client.timeouts, frame_misses,
                            age, led_current_ma, ws2812_port_busy_drops());
    last_diag_ms = now_ms;
  }
  diagnostics_tick(now_ms);
}
