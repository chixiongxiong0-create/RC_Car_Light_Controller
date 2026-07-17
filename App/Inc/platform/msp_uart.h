#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void msp_uart_init(void);
void msp_uart_service(void);
bool msp_uart_read(uint8_t *byte);
bool msp_uart_write(const uint8_t *data, size_t length);
uint32_t msp_uart_overruns(void);
uint32_t msp_uart_rx_arm_failures(void);
uint32_t msp_uart_rx_errors(void);
