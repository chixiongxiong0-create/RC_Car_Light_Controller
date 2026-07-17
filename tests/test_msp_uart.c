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
static unsigned rx_aborts;
static const uint8_t *pending_tx;
static uint16_t pending_tx_length;
static HAL_StatusTypeDef receive_result;
static HAL_StatusTypeDef transmit_result;
static int usart3_token;
static int other_uart_token;
void *const fake_usart3_instance = &usart3_token;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart);

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *uart,
                                      uint8_t *data, uint16_t length)
{
    assert(uart == &huart3);
    assert(length == 1u);
    ++rx_arms;
    armed_rx = receive_result == HAL_OK ? data : NULL;
    return receive_result;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                       const uint8_t *data, uint16_t length)
{
    assert(uart == &huart3);
    pending_tx = data;
    pending_tx_length = length;
    ++tx_starts;
    return transmit_result;
}

HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *uart)
{
    assert(uart == &huart3);
    ++rx_aborts;
    armed_rx = NULL;
    return HAL_OK;
}

static void reset_without_init(void)
{
    huart3.Instance = USART3;
    armed_rx = NULL;
    rx_arms = 0u;
    tx_starts = 0u;
    rx_aborts = 0u;
    pending_tx = NULL;
    pending_tx_length = 0u;
    receive_result = HAL_OK;
    transmit_result = HAL_OK;
}

static void reset_fixture(void)
{
    reset_without_init();
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

static void test_transmit_owns_bytes_until_completion(void)
{
    static const uint8_t expected[6] = {'$', 'M', '<', 0u, 105u, 105u};
    uint8_t request[6];
    reset_fixture();
    memcpy(request, expected, sizeof request);

    assert(msp_uart_write(request, sizeof request));
    memset(request, 0xa5, sizeof request);
    assert(tx_starts == 1u);
    assert(pending_tx_length == sizeof expected);
    assert(memcmp(pending_tx, expected, sizeof expected) == 0);

    assert(!msp_uart_write(request, sizeof request));
    assert(tx_starts == 1u);
    assert(memcmp(pending_tx, expected, sizeof expected) == 0);

    HAL_UART_TxCpltCallback(&huart3);
    assert(msp_uart_write(expected, sizeof expected));
    assert(tx_starts == 2u);
}

static void test_transmit_rejects_more_than_one_msp_request(void)
{
    uint8_t oversized[7] = {0};
    reset_fixture();
    assert(!msp_uart_write(oversized, sizeof oversized));
    assert(tx_starts == 0u);
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

static void test_initial_arm_failure_recovers_from_service(void)
{
    reset_without_init();
    receive_result = HAL_BUSY;
    msp_uart_init();
    assert(rx_arms == 1u);
    assert(msp_uart_rx_arm_failures() == 1u);

    receive_result = HAL_OK;
    msp_uart_service();
    assert(rx_arms == 2u);
    receive_byte(0x42u);
}

static void test_completion_rearm_failure_recovers_from_service(void)
{
    uint8_t byte = 0u;
    reset_fixture();
    receive_result = HAL_BUSY;
    receive_byte(0x33u);
    assert(msp_uart_read(&byte) && byte == 0x33u);
    assert(msp_uart_rx_arm_failures() == 1u);

    receive_result = HAL_OK;
    msp_uart_service();
    receive_byte(0x34u);
    assert(msp_uart_read(&byte) && byte == 0x34u);
}

static void test_error_callback_aborts_then_service_recovers(void)
{
    reset_fixture();
    HAL_UART_ErrorCallback(&huart3);
    assert(msp_uart_rx_errors() == 1u);
    assert(rx_aborts == 0u);

    msp_uart_service();
    assert(rx_aborts == 1u);
    assert(rx_arms == 2u);
    receive_byte(0x55u);
}

static void test_callbacks_ignore_other_uart(void)
{
    UART_HandleTypeDef other = {&other_uart_token};
    static const uint8_t request[6] = {'$', 'M', '<', 0u, 105u, 105u};
    uint8_t byte = 0u;
    reset_fixture();
    const unsigned arms_before = rx_arms;
    assert(msp_uart_write(request, sizeof request));

    HAL_UART_RxCpltCallback(&other);
    HAL_UART_TxCpltCallback(&other);
    HAL_UART_ErrorCallback(&other);
    assert(rx_arms == arms_before);
    assert(msp_uart_rx_errors() == 0u);
    assert(!msp_uart_read(&byte));
    assert(!msp_uart_write(request, sizeof request));
    HAL_UART_TxCpltCallback(&huart3);
}

void test_msp_uart(void)
{
    test_receive_is_armed_and_bytes_are_read_in_order();
    test_full_ring_discards_newest_and_counts_overrun();
    test_transmit_owns_bytes_until_completion();
    test_transmit_rejects_more_than_one_msp_request();
    test_failed_transmit_does_not_leave_guard_busy();
    test_initial_arm_failure_recovers_from_service();
    test_completion_rearm_failure_recovers_from_service();
    test_error_callback_aborts_then_service_recovers();
    test_callbacks_ignore_other_uart();
}
