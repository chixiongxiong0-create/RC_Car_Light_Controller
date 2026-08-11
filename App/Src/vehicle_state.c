#include "vehicle_state.h"

#include <stddef.h>
#include <string.h>

enum {
    MSP_STATUS = 101,
    MSP_RC = 105,
    MSP_RAW_GPS = 106,
    MSP_ATTITUDE = 108,
    MSP_ANALOG = 110,
    STALE_MS = 500,
    LOST_MS = 2000,
    RECOVERY_MS = 500
};

static VehicleState state;
static uint32_t recovery_started_ms;
static bool have_msp;
static bool have_rc;
static bool have_attitude;
static bool recovering;

static bool read_u16(const MspFrame *frame, size_t offset, uint16_t *value)
{
    if (offset > frame->length || (size_t)frame->length - offset < 2u) {
        return false;
    }
    *value = (uint16_t)frame->payload[offset] |
             ((uint16_t)frame->payload[offset + 1u] << 8);
    return true;
}

static bool read_i16(const MspFrame *frame, size_t offset, int16_t *value)
{
    uint16_t raw;
    if (!read_u16(frame, offset, &raw)) {
        return false;
    }
    *value = (int16_t)raw;
    return true;
}

static bool read_u32(const MspFrame *frame, size_t offset, uint32_t *value)
{
    if (offset > frame->length || (size_t)frame->length - offset < 4u) {
        return false;
    }
    *value = (uint32_t)frame->payload[offset] |
             ((uint32_t)frame->payload[offset + 1u] << 8) |
             ((uint32_t)frame->payload[offset + 2u] << 16) |
             ((uint32_t)frame->payload[offset + 3u] << 24);
    return true;
}

static float lowpass(float previous, float input, float alpha)
{
    return previous + alpha * (input - previous);
}

static float normalize_channel(uint16_t pulse)
{
    if (pulse > 2000u) {
        pulse = 2000u;
    } else if (pulse < 1000u) {
        pulse = 1000u;
    }
    float value = ((float)pulse - 1500.0f) / 500.0f;
    if (value > -0.03f && value < 0.03f) {
        value = 0.0f;
    }
    return value;
}

static bool valid_length(const MspFrame *frame)
{
    switch (frame->command) {
    case MSP_RC:
        return frame->length >= 10u && (frame->length & 1u) == 0u;
    case MSP_ATTITUDE:
        return frame->length == 6u;
    case MSP_ANALOG:
        return frame->length >= 7u;
    case MSP_RAW_GPS:
        return frame->length >= 18u;
    case MSP_STATUS:
        return frame->length >= 11u;
    default:
        return false;
    }
}

static void note_valid_frame(uint32_t now_ms)
{
    const LinkState previous_link = state.link;
    const uint32_t previous_age_ms = now_ms - state.last_msp_ms;
    if (previous_link == LINK_LOST && have_msp && previous_age_ms >= STALE_MS) {
        recovering = false;
    }
    state.last_msp_ms = now_ms;
    have_msp = true;

    if (previous_link == LINK_LOST) {
        if (!recovering) {
            recovery_started_ms = now_ms;
            recovering = true;
        }
        if ((uint32_t)(now_ms - recovery_started_ms) >= RECOVERY_MS) {
            state.link = LINK_OK;
            recovering = false;
        }
    } else {
        state.link = LINK_OK;
        recovering = false;
    }
}

void vehicle_state_init(void)
{
    memset(&state, 0, sizeof state);
    state.link = LINK_STARTING;
    state.aux6 = -1.0f;
    state.aux7 = -1.0f;
    state.aux8 = -1.0f;
    state.aux9 = -1.0f;
    recovery_started_ms = 0u;
    have_msp = false;
    have_rc = false;
    have_attitude = false;
    recovering = false;
}

bool vehicle_state_on_msp(const MspFrame *frame, uint32_t now_ms)
{
    if (frame == NULL || !valid_length(frame)) {
        return false;
    }

    const bool freeze_fast = state.link == LINK_LOST;
    note_valid_frame(now_ms);

    switch (frame->command) {
    case MSP_RC: {
        uint16_t raw_steering;
        uint16_t raw_throttle;
        uint16_t raw_aux_page;
        if (!read_u16(frame, 0u, &raw_steering) ||
            !read_u16(frame, 4u, &raw_throttle) ||
            !read_u16(frame, 8u, &raw_aux_page)) {
            return false;
        }
        const float steering = normalize_channel(raw_steering);
        const float throttle = normalize_channel(raw_throttle);
        const float aux_page = normalize_channel(raw_aux_page);
        uint16_t raw_aux6;
        uint16_t raw_aux7;
        uint16_t raw_aux8;
        uint16_t raw_aux9;
        const bool has_lighting_channels = frame->length >= 26u;
        if (has_lighting_channels &&
            (!read_u16(frame, 18u, &raw_aux6) ||
             !read_u16(frame, 20u, &raw_aux7) ||
             !read_u16(frame, 22u, &raw_aux8) ||
             !read_u16(frame, 24u, &raw_aux9))) {
            return false;
        }
        state.last_rc_ms = now_ms;
        if (freeze_fast && state.link != LINK_OK) {
            return true;
        }
        if (!have_rc) {
            state.steering = steering;
            state.throttle = throttle;
            state.aux_page = aux_page;
            have_rc = true;
        } else {
            state.steering = lowpass(state.steering, steering, 0.35f);
            state.throttle = lowpass(state.throttle, throttle, 0.35f);
            state.aux_page = lowpass(state.aux_page, aux_page, 0.35f);
        }
        if (has_lighting_channels) {
            state.aux6 = normalize_channel(raw_aux6);
            state.aux7 = normalize_channel(raw_aux7);
            state.aux8 = normalize_channel(raw_aux8);
            state.aux9 = normalize_channel(raw_aux9);
        } else {
            state.aux6 = -1.0f;
            state.aux7 = -1.0f;
            state.aux8 = -1.0f;
            state.aux9 = -1.0f;
        }
        break;
    }
    case MSP_ATTITUDE: {
        int16_t raw_roll;
        int16_t raw_pitch;
        int16_t raw_heading;
        if (!read_i16(frame, 0u, &raw_roll) ||
            !read_i16(frame, 2u, &raw_pitch) ||
            !read_i16(frame, 4u, &raw_heading)) {
            return false;
        }
        const float roll = (float)raw_roll * 0.1f;
        const float pitch = (float)raw_pitch * 0.1f;
        const float heading = (float)raw_heading;
        state.last_attitude_ms = now_ms;
        if (freeze_fast && state.link != LINK_OK) {
            return true;
        }
        if (!have_attitude) {
            state.roll_deg = roll;
            state.pitch_deg = pitch;
            state.heading_deg = heading;
            have_attitude = true;
        } else {
            state.roll_deg = lowpass(state.roll_deg, roll, 0.25f);
            state.pitch_deg = lowpass(state.pitch_deg, pitch, 0.25f);
            state.heading_deg = lowpass(state.heading_deg, heading, 0.25f);
        }
        break;
    }
    case MSP_ANALOG: {
        uint16_t rssi;
        if (!read_u16(frame, 3u, &rssi)) {
            return false;
        }
        state.battery_v = (float)frame->payload[0] * 0.1f;
        state.rssi = rssi;
        state.battery_valid = true;
        break;
    }
    case MSP_RAW_GPS:
        state.gps_sats = frame->payload[1];
        break;
    case MSP_STATUS: {
        uint32_t mode_flags;
        if (!read_u32(frame, 6u, &mode_flags)) {
            return false;
        }
        state.mode_flags = mode_flags;
        state.armed = (mode_flags & 1u) != 0u;
        break;
    }
    default:
        return false;
    }
    return true;
}

void vehicle_state_tick(uint32_t now_ms)
{
    if (!have_msp) {
        return;
    }
    const uint32_t age_ms = now_ms - state.last_msp_ms;
    if (age_ms >= LOST_MS) {
        state.link = LINK_LOST;
        recovering = false;
    } else if (state.link == LINK_LOST) {
        if (age_ms >= STALE_MS) {
            recovering = false;
        }
    } else if (age_ms >= STALE_MS) {
        state.link = LINK_STALE;
    }
}

const VehicleState *vehicle_state_get(void)
{
    return &state;
}
