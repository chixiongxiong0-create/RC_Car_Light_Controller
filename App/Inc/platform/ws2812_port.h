#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "led/led_controller.h"

#define WS2812_BYTES_PER_PIXEL 12u
#define WS2812_ENCODED_BYTES (LED_MAX_PIXELS * WS2812_BYTES_PER_PIXEL)
#define WS2812_RESET_BYTES 24u
#define WS2812_TX_BYTES (WS2812_ENCODED_BYTES + WS2812_RESET_BYTES)

size_t ws2812_encode_grb(const LedRgb *pixels, size_t count,
                         uint8_t *out, size_t capacity);
bool ws2812_can_submit(uint32_t now_ms, uint32_t last_submit_ms, bool idle);

#ifndef WS2812_HOST_TEST
void ws2812_port_init(void);
bool ws2812_port_submit(uint32_t now_ms, const LedRgb *pixels, size_t count);
void ws2812_port_tx_complete(void *spi_handle);
#endif
