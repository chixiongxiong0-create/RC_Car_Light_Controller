#include "platform/msp_uart.h"

#include <string.h>

#include "usart.h"

enum {
    RX_RING_SIZE = 256u,
    RX_RING_MASK = RX_RING_SIZE - 1u,
    TX_BUFFER_SIZE = 6u
};

static uint8_t rx_ring[RX_RING_SIZE];
static uint8_t rx_byte;
static uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile uint32_t rx_overruns;
static volatile uint32_t rx_arm_failures;
static volatile uint32_t rx_errors;
static volatile bool rx_armed;
static volatile bool rx_recovery_requested;
static volatile bool tx_busy;

#ifdef MSP_UART_HOST_TEST
extern uint32_t msp_uart_test_critical_enter(void);
extern void msp_uart_test_critical_exit(uint32_t saved_primask);
#define critical_enter msp_uart_test_critical_enter
#define critical_exit msp_uart_test_critical_exit
#else
static uint32_t critical_enter(void)
{
    const uint32_t saved_primask = __get_PRIMASK();
    __disable_irq();
    __DMB();
    return saved_primask;
}

static void critical_exit(uint32_t saved_primask)
{
    __DMB();
    __set_PRIMASK(saved_primask);
}
#endif

static bool arm_receive_once(void)
{
    const uint32_t saved_primask = critical_enter();
    const bool armed = HAL_UART_Receive_IT(&huart3, &rx_byte, 1u) == HAL_OK;
    rx_armed = armed;
    if (!armed) {
        ++rx_arm_failures;
    }
    critical_exit(saved_primask);
    return armed;
}

void msp_uart_init(void)
{
    rx_head = 0u;
    rx_tail = 0u;
    rx_overruns = 0u;
    rx_arm_failures = 0u;
    rx_errors = 0u;
    rx_armed = false;
    rx_recovery_requested = false;
    tx_busy = false;
    (void)arm_receive_once();
}

void msp_uart_service(void)
{
    if (rx_recovery_requested) {
        /* This HAL API synchronously resets RX state; it performs no polling. */
        if (HAL_UART_AbortReceive(&huart3) != HAL_OK) {
            return;
        }
        rx_recovery_requested = false;
    }

    if (!rx_armed) {
        (void)arm_receive_once();
    }
}

bool msp_uart_read(uint8_t *byte)
{
    const uint8_t tail = rx_tail;
    if (byte == NULL || tail == rx_head) {
        return false;
    }

    *byte = rx_ring[tail];
    rx_tail = (uint8_t)((tail + 1u) & RX_RING_MASK);
    return true;
}

bool msp_uart_write(const uint8_t *data, size_t length)
{
    if (data == NULL || length == 0u || length > sizeof tx_buffer || tx_busy) {
        return false;
    }

    tx_busy = true;
    memcpy(tx_buffer, data, length);
    if (HAL_UART_Transmit_IT(&huart3, tx_buffer, (uint16_t)length) != HAL_OK) {
        tx_busy = false;
        return false;
    }
    return true;
}

uint32_t msp_uart_overruns(void)
{
    return rx_overruns;
}

uint32_t msp_uart_rx_arm_failures(void)
{
    return rx_arm_failures;
}

uint32_t msp_uart_rx_errors(void)
{
    return rx_errors;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart3 && uart->Instance == USART3) {
        rx_armed = false;
        const uint8_t head = rx_head;
        const uint8_t next = (uint8_t)((head + 1u) & RX_RING_MASK);
        if (next == rx_tail) {
            ++rx_overruns;
        } else {
            rx_ring[head] = rx_byte;
            rx_head = next;
        }
        (void)arm_receive_once();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart3 && uart->Instance == USART3) {
        ++rx_errors;
        rx_armed = false;
        rx_recovery_requested = true;
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart3 && uart->Instance == USART3) {
        tx_busy = false;
    }
}
