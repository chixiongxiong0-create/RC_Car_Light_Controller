#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "platform/msp_uart.h"
#include "usart.h"

UART_HandleTypeDef huart3;
static uint8_t *armed_rx;
static unsigned rx_arms;
static unsigned tx_starts;
static uint8_t transmitted[16];
static uint16_t transmitted_length;
static HAL_StatusTypeDef receive_result;
static HAL_StatusTypeDef transmit_result;
static int usart3_token;
void *const fake_usart3_instance = &usart3_token;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart);

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *uart,
                                      uint8_t *data, uint16_t length)
{
    assert(uart == &huart3);
    assert(length == 1u);
    armed_rx = data;
    ++rx_arms;
    return receive_result;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                       const uint8_t *data, uint16_t length)
{
    assert(uart == &huart3);
    assert(length <= sizeof transmitted);
    memcpy(transmitted, data, length);
    transmitted_length = length;
    ++tx_starts;
    return transmit_result;
}

static void reset_fixture(void)
{
    huart3.Instance = USART3;
    armed_rx = NULL;
    rx_arms = 0u;
    tx_starts = 0u;
    transmitted_length = 0u;
    receive_result = HAL_OK;
    transmit_result = HAL_OK;
    msp_uart_init();
}

static void receive_byte(uint8_t byte)
{
    assert(armed_rx != NULL);
    *armed_rx = byte;
    HAL_UART_RxCpltCallback(&huart3);
}

static void test_receive_is_armed_and_bytes_are_read_in_order(void)
{
    uint8_t byte = 0u;
    reset_fixture();
    assert(rx_arms == 1u);

    receive_byte(0x24u);
    receive_byte(0x4du);
    assert(rx_arms == 3u);
    assert(msp_uart_read(&byte) && byte == 0x24u);
    assert(msp_uart_read(&byte) && byte == 0x4du);
    assert(!msp_uart_read(&byte));
}

static void test_full_ring_discards_newest_and_counts_overrun(void)
{
    uint8_t byte = 0u;
    reset_fixture();

    for (unsigned i = 0u; i < 255u; ++i) {
        receive_byte((uint8_t)i);
    }
    receive_byte(0xeeu);
    assert(msp_uart_overruns() == 1u);

    for (unsigned i = 0u; i < 255u; ++i) {
        assert(msp_uart_read(&byte));
        assert(byte == (uint8_t)i);
    }
    assert(!msp_uart_read(&byte));
}

static void test_transmit_guard_rejects_busy_until_completion(void)
{
    static const uint8_t request[6] = {'$', 'M', '<', 0u, 105u, 105u};
    reset_fixture();

    assert(msp_uart_write(request, sizeof request));
    assert(tx_starts == 1u);
    assert(transmitted_length == sizeof request);
    assert(memcmp(transmitted, request, sizeof request) == 0);
    assert(!msp_uart_write(request, sizeof request));
    assert(tx_starts == 1u);

    HAL_UART_TxCpltCallback(&huart3);
    assert(msp_uart_write(request, sizeof request));
    assert(tx_starts == 2u);
}

static void test_failed_transmit_does_not_leave_guard_busy(void)
{
    static const uint8_t request[6] = {'$', 'M', '<', 0u, 108u, 108u};
    reset_fixture();
    transmit_result = HAL_BUSY;
    assert(!msp_uart_write(request, sizeof request));
    transmit_result = HAL_OK;
    assert(msp_uart_write(request, sizeof request));
    assert(tx_starts == 2u);
}

void test_msp_uart(void)
{
    test_receive_is_armed_and_bytes_are_read_in_order();
    test_full_ring_discards_newest_and_counts_overrun();
    test_transmit_guard_rejects_busy_until_completion();
    test_failed_transmit_does_not_leave_guard_busy();
}
