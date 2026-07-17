#pragma once

/* Set to the vehicle battery's series cell count (2..6) before flashing.
 * Zero means unknown: low-battery expressions are deliberately disabled. */
#ifndef APP_BATTERY_CELL_COUNT
#define APP_BATTERY_CELL_COUNT 0
#endif

#if APP_BATTERY_CELL_COUNT != 0 && \
    (APP_BATTERY_CELL_COUNT < 2 || APP_BATTERY_CELL_COUNT > 6)
#error "APP_BATTERY_CELL_COUNT must be 0 (unknown) or 2..6"
#endif
