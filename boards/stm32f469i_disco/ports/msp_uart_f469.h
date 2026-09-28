#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool f469_msp_uart_init(void);
bool f469_msp_uart_read(uint8_t *byte);
bool f469_msp_uart_write(const uint8_t *bytes, size_t count);
uint32_t f469_msp_uart_overruns(void);
