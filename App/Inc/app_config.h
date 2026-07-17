#pragma once

/* Set to the vehicle battery's series cell count (2..6) before flashing.
 * Zero means unknown: low-battery expressions are deliberately disabled. */
#ifndef APP_BATTERY_CELL_COUNT
#define APP_BATTERY_CELL_COUNT 0
#endif

#ifndef APP_LED_PIXEL_COUNT
#define APP_LED_PIXEL_COUNT 30
#endif

#if APP_LED_PIXEL_COUNT < 10 || APP_LED_PIXEL_COUNT > 30
#error "APP_LED_PIXEL_COUNT must be 10..30"
#endif

#if APP_BATTERY_CELL_COUNT != 0 && \
    (APP_BATTERY_CELL_COUNT < 2 || APP_BATTERY_CELL_COUNT > 6)
#error "APP_BATTERY_CELL_COUNT must be 0 (unknown) or 2..6"
#endif
