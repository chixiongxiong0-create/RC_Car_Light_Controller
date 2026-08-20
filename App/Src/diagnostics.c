#include "diagnostics.h"

#include <string.h>

#include "platform/msp_uart.h"

#ifndef DIAGNOSTICS_HOST_TEST
#include "iwdg.h"
#include "main.h"
#endif

static DiagnosticsSnapshot snapshot;
static DiagnosticsInputs inputs;
static DiagnosticsWatchdogGate watchdog_gate;
static bool watchdog_started;

HealthState diagnostics_health(const DiagnosticsInputs *value)
{
    if (value == NULL || value->init_failed) {
        return HEALTH_FAULT;
    }
    if (!value->init_complete) {
        return HEALTH_BOOTING;
    }
    if (value->uart_overruns != 0u || value->frame_misses >= 3u) {
        return HEALTH_DEGRADED;
    }
    return HEALTH_OK;
}

void diagnostics_watchdog_gate_init(DiagnosticsWatchdogGate *gate)
{
    if (gate != NULL) {
        gate->progress = 0u;
    }
}

void diagnostics_watchdog_progress(DiagnosticsWatchdogGate *gate, uint8_t progress)
{
    if (gate != NULL) {
        gate->progress |= progress & DIAG_PROGRESS_ALL;
    }
}

bool diagnostics_watchdog_take_refresh(DiagnosticsWatchdogGate *gate)
{
    if (gate == NULL || gate->progress != DIAG_PROGRESS_ALL) {
        return false;
    }
    gate->progress = 0u;
    return true;
}

void diagnostics_init(void)
{
    memset(&snapshot, 0, sizeof snapshot);
    memset(&inputs, 0, sizeof inputs);
    snapshot.state = HEALTH_BOOTING;
    diagnostics_watchdog_gate_init(&watchdog_gate);
    watchdog_started = false;
#ifndef DIAGNOSTICS_HOST_TEST
    snapshot.reset_flags = RCC->RSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
#endif
}

void diagnostics_tick(uint32_t now_ms)
{
    (void)now_ms;
    snapshot.uart_overruns = msp_uart_overruns();
    inputs.uart_overruns = snapshot.uart_overruns;
    inputs.frame_misses = snapshot.frame_misses;
    snapshot.state = diagnostics_health(&inputs);
}

void diagnostics_set_init_result(bool success)
{
    inputs.init_complete = success;
    inputs.init_failed = !success;
    snapshot.state = diagnostics_health(&inputs);
}

void diagnostics_set_runtime(uint16_t fps, uint32_t loop_us,
                             uint32_t msp_timeouts, uint32_t frame_misses,
                             uint32_t msp_age_ms, uint32_t led_current_ma,
                             uint32_t ws2812_busy_drops)
{
    snapshot.fps = fps;
    if (loop_us > snapshot.max_loop_us) {
        snapshot.max_loop_us = loop_us;
    }
    snapshot.msp_timeouts = msp_timeouts;
    snapshot.frame_misses = frame_misses;
    snapshot.msp_age_ms = msp_age_ms;
    snapshot.led_current_ma = led_current_ma;
    snapshot.ws2812_busy_drops = ws2812_busy_drops;
}

void diagnostics_set_touch_available(bool available)
{
    snapshot.touch_available = available;
}

void diagnostics_watchdog_apply_start_result(WatchdogStartResult result)
{
    watchdog_started = result != WATCHDOG_NOT_STARTED_FAIL;
    if (result != WATCHDOG_STARTED_OK) {
        inputs.init_failed = true;
        snapshot.state = diagnostics_health(&inputs);
    }
}

bool diagnostics_watchdog_is_started(void)
{
    return watchdog_started;
}

bool diagnostics_watchdog_start(void)
{
#ifndef DIAGNOSTICS_HOST_TEST
    if (snapshot.state == HEALTH_FAULT) {
        return false;
    }
    const WatchdogStartResult result = MX_IWDG1_Init();
    diagnostics_watchdog_apply_start_result(result);
    return result == WATCHDOG_STARTED_OK;
#else
    return false;
#endif
}

void diagnostics_watchdog_mark(uint8_t progress)
{
    diagnostics_watchdog_progress(&watchdog_gate, progress);
    if (diagnostics_watchdog_take_refresh(&watchdog_gate)) {
#ifndef DIAGNOSTICS_HOST_TEST
        if (watchdog_started) {
            IWDG1_Refresh();
        }
#endif
    }
}

const DiagnosticsSnapshot *diagnostics_get(void)
{
    return &snapshot;
}
