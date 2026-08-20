#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/watchdog_status.h"

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
    uint32_t msp_age_ms;
    uint32_t led_current_ma;
    uint32_t ws2812_busy_drops;
    uint32_t reset_flags;
    bool touch_available;
} DiagnosticsSnapshot;

typedef struct {
    bool init_complete;
    bool init_failed;
    uint32_t uart_overruns;
    uint32_t frame_misses;
} DiagnosticsInputs;

enum {
    DIAG_PROGRESS_MSP = 1u << 0,
    DIAG_PROGRESS_UI = 1u << 1,
    DIAG_PROGRESS_LED = 1u << 2,
    DIAG_PROGRESS_ALL = DIAG_PROGRESS_MSP | DIAG_PROGRESS_UI | DIAG_PROGRESS_LED
};

typedef struct {
    uint8_t progress;
} DiagnosticsWatchdogGate;

HealthState diagnostics_health(const DiagnosticsInputs *inputs);
void diagnostics_watchdog_gate_init(DiagnosticsWatchdogGate *gate);
void diagnostics_watchdog_progress(DiagnosticsWatchdogGate *gate, uint8_t progress);
bool diagnostics_watchdog_take_refresh(DiagnosticsWatchdogGate *gate);

void diagnostics_init(void);
void diagnostics_tick(uint32_t now_ms);
void diagnostics_set_init_result(bool success);
void diagnostics_set_runtime(uint16_t fps, uint32_t loop_us,
                             uint32_t msp_timeouts, uint32_t frame_misses,
                             uint32_t msp_age_ms, uint32_t led_current_ma,
                             uint32_t ws2812_busy_drops);
void diagnostics_set_touch_available(bool available);
bool diagnostics_watchdog_start(void);
void diagnostics_watchdog_apply_start_result(WatchdogStartResult result);
bool diagnostics_watchdog_is_started(void);
void diagnostics_watchdog_mark(uint8_t progress);
const DiagnosticsSnapshot *diagnostics_get(void);
