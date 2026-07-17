#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HEALTH_BOOTING,
    HEALTH_OK,
    HEALTH_DEGRADED,
    HEALTH_FAULT
} HealthState;

typedef struct {
    HealthState state;
    uint16_t fps;
    uint32_t max_loop_us;
    uint32_t msp_timeouts;
    uint32_t uart_overruns;
    uint32_t frame_misses;
    bool touch_available;
} DiagnosticsSnapshot;

void diagnostics_init(void);
void diagnostics_tick(uint32_t now_ms);
const DiagnosticsSnapshot *diagnostics_get(void);
