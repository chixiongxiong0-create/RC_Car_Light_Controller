#pragma once

#include <stddef.h>
#include <stdint.h>

#define LED_MAX_PIXELS 30u
#define LED_CURRENT_BUDGET_MA 1000u

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} LedRgb;

uint32_t led_estimated_ma(const LedRgb *pixels, size_t count);
void led_limit_current(LedRgb *pixels, size_t count, uint32_t budget_ma);
