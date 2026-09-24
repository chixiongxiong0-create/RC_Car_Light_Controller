#include "msp_uart_f469.h"

#include <string.h>

#include "stm32f4xx_hal.h"

enum { RX_CAPACITY = 256, TX_CAPACITY = 64 };

static UART_HandleTypeDef uart;
static uint8_t rx_byte;
static uint8_t rx_queue[RX_CAPACITY];
static uint8_t tx_queue[TX_CAPACITY];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static volatile uint32_t overruns;
static volatile bool tx_busy;

bool f469_msp_uart_init(void)
{
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART6_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_14 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOG, &gpio);

    uart.Instance = USART6;
    uart.Init.BaudRate = 115200;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&uart) != HAL_OK) return false;
    HAL_NVIC_SetPriority(USART6_IRQn, 5u, 0u);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    rx_head = rx_tail = 0u;
    overruns = 0u;
    tx_busy = false;
    return HAL_UART_Receive_IT(&uart, &rx_byte, 1u) == HAL_OK;
}

bool f469_msp_uart_read(uint8_t *byte)
{
    if (byte == NULL || rx_tail == rx_head) return false;
    *byte = rx_queue[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1u) % RX_CAPACITY);
    return true;
}

bool f469_msp_uart_write(const uint8_t *bytes, size_t count)
{
    if (bytes == NULL || count == 0u || count > TX_CAPACITY || tx_busy)
        return false;
    memcpy(tx_queue, bytes, count);
    tx_busy = true;
    if (HAL_UART_Transmit_IT(&uart, tx_queue, (uint16_t)count) != HAL_OK) {
        tx_busy = false;
        return false;
    }
    return true;
}

uint32_t f469_msp_uart_overruns(void)
{
    return overruns;
}

void USART6_IRQHandler(void)
{
    HAL_UART_IRQHandler(&uart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    if (handle != &uart) return;
    const uint16_t next = (uint16_t)((rx_head + 1u) % RX_CAPACITY);
    if (next == rx_tail) ++overruns;
    else {
        rx_queue[rx_head] = rx_byte;
        rx_head = next;
    }
    if (HAL_UART_Receive_IT(&uart, &rx_byte, 1u) != HAL_OK) ++overruns;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
    if (handle == &uart) tx_busy = false;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
    if (handle != &uart) return;
    ++overruns;
    tx_busy = false;
    (void)HAL_UART_Receive_IT(&uart, &rx_byte, 1u);
}
