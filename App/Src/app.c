#include "app.h"

#include "diagnostics.h"
#include "msp/msp_client.h"
#include "platform/msp_uart.h"
#include "usart.h"
#include "vehicle_state.h"

static MspClient client;

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

/* Task 5 adapters: replaced by their owning modules in later tasks. */
static void input_manager_tick(uint32_t now_ms, const VehicleState *state)
{
  (void)now_ms;
  (void)state;
}

static void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
  (void)now_ms;
  (void)state;
}

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
  msp_client_init(&client, write_msp, on_msp_frame, NULL);
  diagnostics_init();
}

void App_Tick(uint32_t now_ms)
{
  uint8_t byte;
  while (msp_uart_read(&byte)) {
    msp_client_rx_byte(&client, byte, now_ms);
  }
  msp_client_tick(&client, now_ms);
  vehicle_state_tick(now_ms);
  input_manager_tick(now_ms, vehicle_state_get());
  ui_app_tick(now_ms, vehicle_state_get());
  led_controller_tick(now_ms, vehicle_state_get());
  diagnostics_tick(now_ms);
}
