#include "platform/msp_uart.h"

#include <limits.h>

#include "usart.h"

enum {
    RX_RING_SIZE = 256u,
    RX_RING_MASK = RX_RING_SIZE - 1u
};

static uint8_t rx_ring[RX_RING_SIZE];
static uint8_t rx_byte;
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile uint32_t rx_overruns;
static volatile bool tx_busy;

void msp_uart_init(void)
{
    rx_head = 0u;
    rx_tail = 0u;
    rx_overruns = 0u;
    tx_busy = false;
    (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1u);
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
    if (data == NULL || length == 0u || length > UINT16_MAX || tx_busy) {
        return false;
    }

    tx_busy = true;
    if (HAL_UART_Transmit_IT(&huart3, (uint8_t *)data, (uint16_t)length) != HAL_OK) {
        tx_busy = false;
        return false;
    }
    return true;
}

uint32_t msp_uart_overruns(void)
{
    return rx_overruns;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart3 && uart->Instance == USART3) {
        const uint8_t head = rx_head;
        const uint8_t next = (uint8_t)((head + 1u) & RX_RING_MASK);
        if (next == rx_tail) {
            ++rx_overruns;
        } else {
            rx_ring[head] = rx_byte;
            rx_head = next;
        }
        (void)HAL_UART_Receive_IT(&huart3, &rx_byte, 1u);
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart3 && uart->Instance == USART3) {
        tx_busy = false;
    }
}
