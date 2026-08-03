#include "demo_vehicle_state.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define DEMO_PI 3.14159265358979323846f

static float sine_phase(uint32_t phase_ms, uint32_t period_ms)
{
    const float angle = (float)phase_ms * (2.0f * DEMO_PI / (float)period_ms);
    return sinf(angle);
}

void demo_vehicle_state_sample(uint32_t now_ms, uint8_t battery_cells,
                               VehicleState *out)
{
    const uint32_t phase_ms = now_ms % 12000u;
    const uint8_t cells = battery_cells == 0u ? 4u : battery_cells;

    if (out == NULL) {
        return;
    }

    memset(out, 0, sizeof *out);
    out->throttle = 0.20f + 0.55f * sine_phase(phase_ms, 8000u);
    out->steering = 0.65f * sine_phase(phase_ms, 6000u);
    out->aux_page = 0.0f;
    out->roll_deg = 12.0f * sine_phase(phase_ms, 4000u);
    out->pitch_deg = 8.0f * sine_phase((phase_ms + 1000u) % 12000u, 6000u);
    out->heading_deg = (float)((now_ms / 20u) % 360u);
    out->battery_v = (float)cells *
                     (3.95f + 0.05f * sine_phase(phase_ms, 12000u));
    out->rssi = (uint16_t)(760u + ((now_ms / 250u) % 7u) * 25u);
    out->mode_flags = 0u;
    out->gps_sats = (uint8_t)(9u + ((now_ms / 1000u) % 5u));
    out->armed = false;
    out->link = LINK_STARTING;
    out->last_msp_ms = now_ms;
    out->last_rc_ms = now_ms;
    out->last_attitude_ms = now_ms;
}
