#include "app.h"

#include "diagnostics.h"
#include "input_manager.h"
#include "msp/msp_client.h"
#include "platform/msp_uart.h"
#ifdef BSP_CONFIG_SEEDSTUDIO
#include "platform/lvgl_port.h"
#include "ui/ui_app.h"
#endif
#include "usart.h"
#include "vehicle_state.h"

static MspClient client;
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

/* Task 5 LED adapter: replaced by its owning module in Task 10. */
static void led_controller_tick(uint32_t now_ms, const VehicleState *state)
{
  (void)now_ms;
  (void)state;
}

void App_Init(void)
{
  MX_USART3_UART_Init();
  msp_uart_init();
  vehicle_state_init();
  input_manager_init();
  msp_client_init(&client, write_msp, on_msp_frame, NULL);
  diagnostics_init();
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
  msp_uart_service();
  while (msp_uart_read(&byte)) {
    msp_client_rx_byte(&client, byte, now_ms);
  }
  msp_client_tick(&client, now_ms);
  vehicle_state_tick(now_ms);
  input_manager_set_touch_available(diagnostics_get()->touch_available);
  input_manager_tick(now_ms, vehicle_state_get());
#ifdef BSP_CONFIG_SEEDSTUDIO
  if (ui_ready) {
    lvgl_port_tick(now_ms);
    ui_app_tick(now_ms, vehicle_state_get());
  }
#endif
  led_controller_tick(now_ms, vehicle_state_get());
  diagnostics_tick(now_ms);
}
