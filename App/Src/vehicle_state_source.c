#include "vehicle_state_source.h"

#include <stddef.h>
#include <string.h>

#include "demo_vehicle_state.h"

enum { BLEND_DURATION_MS = 300u };

static float lerp(float from, float to, float progress)
{
    return from + (to - from) * progress;
}

static float heading_delta(float from, float to)
{
    float delta = to - from;

    while (delta > 180.0f) {
        delta -= 360.0f;
    }
    while (delta < -180.0f) {
        delta += 360.0f;
    }
    return delta;
}

static float normalize_heading(float heading)
{
    float result = heading;

    while (result >= 360.0f) {
        result -= 360.0f;
    }
    while (result < 0.0f) {
        result += 360.0f;
    }
    return result;
}

void vehicle_state_source_init(VehicleStateSource *source)
{
    if (source == NULL) {
        return;
    }

    memset(source, 0, sizeof *source);
    source->demo.link = LINK_STARTING;
    source->selected = source->demo;
}

void vehicle_state_source_note_real(VehicleStateSource *source, uint32_t now_ms)
{
    if (source == NULL || source->real_seen) {
        return;
    }

    source->real_seen = true;
    source->blending = true;
    source->heading_target_valid = false;
    source->blend_started_ms = now_ms;
    source->blend_from = source->selected;
}

void vehicle_state_source_tick(VehicleStateSource *source, uint32_t now_ms,
                               uint8_t battery_cells,
                               const VehicleState *real_state)
{
    if (source == NULL) {
        return;
    }

    if (!source->real_seen) {
        demo_vehicle_state_sample(now_ms, battery_cells, &source->demo);
        source->selected = source->demo;
        return;
    }

    if (real_state == NULL) {
        return;
    }

    source->selected = *real_state;
    source->selected.heading_deg = normalize_heading(real_state->heading_deg);
    if (source->blending) {
        const uint32_t elapsed_ms = now_ms - source->blend_started_ms;
        const float progress = elapsed_ms >= BLEND_DURATION_MS
                                   ? 1.0f
                                   : (float)elapsed_ms / (float)BLEND_DURATION_MS;

        source->selected.throttle = lerp(source->blend_from.throttle,
                                         real_state->throttle, progress);
        source->selected.steering = lerp(source->blend_from.steering,
                                         real_state->steering, progress);
        source->selected.roll_deg = lerp(source->blend_from.roll_deg,
                                         real_state->roll_deg, progress);
        source->selected.pitch_deg = lerp(source->blend_from.pitch_deg,
                                          real_state->pitch_deg, progress);
        if (!source->heading_target_valid) {
            source->real_heading_unwrapped =
                source->blend_from.heading_deg +
                heading_delta(source->blend_from.heading_deg,
                              real_state->heading_deg);
            source->heading_target_valid = true;
        } else {
            source->real_heading_unwrapped +=
                heading_delta(source->real_heading_unwrapped,
                              real_state->heading_deg);
        }
        source->selected.heading_deg = normalize_heading(
            lerp(source->blend_from.heading_deg,
                 source->real_heading_unwrapped, progress));
        source->selected.battery_v = lerp(source->blend_from.battery_v,
                                          real_state->battery_v, progress);
        if (elapsed_ms >= BLEND_DURATION_MS) {
            source->blending = false;
        }
    }
}

const VehicleState *vehicle_state_source_get(const VehicleStateSource *source)
{
    return source == NULL ? NULL : &source->selected;
}

bool vehicle_state_source_is_demo(const VehicleStateSource *source)
{
    return source != NULL && !source->real_seen;
}
