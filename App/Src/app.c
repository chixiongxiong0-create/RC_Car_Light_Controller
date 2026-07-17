#include "app.h"

#include "diagnostics.h"
#include "app_config.h"
#include "led/led_controller.h"
#include "input_manager.h"
#include "msp/msp_client.h"
#include "platform/msp_uart.h"
#include "platform/button_input.h"
#include "platform/ws2812_port.h"
#include "ui/low_battery_policy.h"
#ifdef BSP_CONFIG_SEEDSTUDIO
#include "platform/lvgl_port.h"
#include "ui/ui_app.h"
#endif
#include "usart.h"
#include "spi.h"
#include "vehicle_state.h"
#include "wio_lite_ai.h"

static MspClient client;
static LowBatteryPolicy battery_policy;
#ifdef BSP_CONFIG_SEEDSTUDIO
static bool ui_ready;
#endif

static bool write_msp(const uint8_t *data, size_t length, void *ctx)
{
  (void)ctx;
  return msp_uart_write(data, length);
}

static void on_msp_frame(const MspFrame *frame, uint32_t now_ms, void *ctx)
{
  (void)ctx;
  (void)vehicle_state_on_msp(frame, now_ms);
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

static void led_controller_tick(uint32_t now_ms, const VehicleState *state,
                                bool low_battery)
{
  LedRgb pixels[LED_MAX_PIXELS];
  const bool board_fault = diagnostics_get()->state == HEALTH_FAULT;
  led_controller_render(now_ms, state, input_manager_page(), low_battery,
                        board_fault, pixels, APP_LED_PIXEL_COUNT);
  led_limit_current(pixels, APP_LED_PIXEL_COUNT, LED_CURRENT_BUDGET_MA);
  (void)ws2812_port_submit(now_ms, pixels, APP_LED_PIXEL_COUNT);
}

void App_Init(void)
{
  MX_USART3_UART_Init();
  msp_uart_init();
  vehicle_state_init();
  input_manager_init();
  low_battery_policy_init(&battery_policy, APP_BATTERY_CELL_COUNT);
  msp_client_init(&client, write_msp, on_msp_frame, NULL);
  diagnostics_init();
  ws2812_port_init();
  input_manager_set_touch_available(diagnostics_get()->touch_available);
#ifdef BSP_CONFIG_SEEDSTUDIO
  ui_ready = lvgl_port_init();
  if (ui_ready) {
    ui_app_init();
  }
#endif
}

void App_Tick(uint32_t now_ms)
{
  uint8_t byte;
  bool low_battery;
  msp_uart_service();
  while (msp_uart_read(&byte)) {
    msp_client_rx_byte(&client, byte, now_ms);
  }
  msp_client_tick(&client, now_ms);
  vehicle_state_tick(now_ms);
  button_input_poll(now_ms, read_user_button, set_user_button, NULL);
  input_manager_set_touch_available(diagnostics_get()->touch_available);
  input_manager_tick(now_ms, vehicle_state_get());
  low_battery = low_battery_policy_update(
      &battery_policy, vehicle_state_get()->battery_v,
      vehicle_state_get()->battery_v > 0.0f);
#ifdef BSP_CONFIG_SEEDSTUDIO
  if (ui_ready) {
    lvgl_port_tick(now_ms);
    ui_app_tick(now_ms, vehicle_state_get(), low_battery);
  }
#endif
  led_controller_tick(now_ms, vehicle_state_get(), low_battery);
  diagnostics_tick(now_ms);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi == &hspi3) {
    ws2812_port_tx_complete(hspi);
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  ws2812_port_error(hspi);
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
  ws2812_port_abort_complete(hspi);
}
