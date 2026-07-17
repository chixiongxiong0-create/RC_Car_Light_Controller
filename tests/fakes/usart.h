#pragma once

#include <stdint.h>

typedef struct {
    void *Instance;
} UART_HandleTypeDef;

typedef enum {
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2
} HAL_StatusTypeDef;

extern UART_HandleTypeDef huart3;
extern void *const fake_usart3_instance;

#define USART3 fake_usart3_instance

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *uart,
                                      uint8_t *data, uint16_t length);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *uart,
                                       const uint8_t *data, uint16_t length);
