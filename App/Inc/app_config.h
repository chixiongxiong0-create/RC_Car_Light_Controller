#pragma once

#include "led/led_controller.h"

/* Vehicle battery series cell count. This hardware uses a 3S pack.
 * Override with 0 only when unknown; low-battery expressions are then disabled. */
#ifndef APP_BATTERY_CELL_COUNT
#define APP_BATTERY_CELL_COUNT 3
#endif

#ifndef APP_LED_PIXEL_COUNT
#define APP_LED_PIXEL_COUNT 30
#endif

#ifndef APP_REAR_PIXEL_COUNT
#define APP_REAR_PIXEL_COUNT 4
#endif

#if APP_REAR_PIXEL_COUNT != 4
#error "APP_REAR_PIXEL_COUNT must be exactly 4"
#endif

#if APP_LED_PIXEL_COUNT < 4 || APP_LED_PIXEL_COUNT > LED_MAX_PIXELS
#error "APP_LED_PIXEL_COUNT must be 4..LED_MAX_PIXELS"
#endif

#if APP_BATTERY_CELL_COUNT != 0 && \
    (APP_BATTERY_CELL_COUNT < 2 || APP_BATTERY_CELL_COUNT > 6)
#error "APP_BATTERY_CELL_COUNT must be 0 (unknown) or 2..6"
#endif
