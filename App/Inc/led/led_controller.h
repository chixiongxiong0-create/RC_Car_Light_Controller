#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ui/ui_types.h"
#include "vehicle_state.h"

#define LED_MAX_PIXELS 30u
#define LED_DEFAULT_PIXELS 30u
#define LED_CURRENT_BUDGET_MA 1000u

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} LedRgb;

typedef enum {
    LED_NORMAL,
    LED_REVERSE,
    LED_TURN_LEFT,
    LED_TURN_RIGHT,
    LED_LOW_BATTERY,
    LED_LINK_LOST,
    LED_BOARD_FAULT
} LedMode;

void led_controller_render(uint32_t now_ms, const VehicleState *state,
                           UiPage page, bool low_battery, bool board_fault,
                           LedRgb out[LED_MAX_PIXELS], size_t count);
uint32_t led_estimated_ma(const LedRgb *pixels, size_t count);
void led_limit_current(LedRgb *pixels, size_t count, uint32_t budget_ma);
