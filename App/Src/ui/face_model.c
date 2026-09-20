#include "ui/screen_face.h"

static float clamp_unit(float value)
{
    if (value < -1.0f) {
        return -1.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

FaceBatteryModel face_battery_model(float battery_v, bool voltage_valid,
                                    uint8_t cell_count)
{
    FaceBatteryModel model = {0u, 0u, false, false};
    if (!voltage_valid || !(battery_v > 0.0f) || cell_count < 2u ||
        cell_count > 6u) {
        return model;
    }

    const float per_cell_v = battery_v / (float)cell_count;
    if (per_cell_v <= 3.50f) {
        model.percent = per_cell_v > 0.0f ? 10u : 0u;
    } else if (per_cell_v >= 4.20f) {
        model.percent = 100u;
    } else {
        model.percent = (uint8_t)(10.0f +
                        ((per_cell_v - 3.50f) * 90.0f / 0.70f) + 0.5f);
    }

    if (model.percent > 85u) {
        model.segments = 6u;
    } else if (model.percent > 70u) {
        model.segments = 5u;
    } else if (model.percent > 55u) {
        model.segments = 4u;
    } else if (model.percent > 40u) {
        model.segments = 3u;
    } else if (model.percent > 25u) {
        model.segments = 2u;
    } else if (model.percent > 10u) {
        model.segments = 1u;
    }
    model.valid = true;
    model.low = model.percent <= 10u;
    return model;
}

FaceMotionModel face_motion_model(uint32_t now_ms, uint8_t aperture,
                                  FaceMood mood)
{
    static const int8_t wave[16] = {
        0, 1, 1, 2, 2, 2, 1, 1, 0, -1, -1, -2, -2, -2, -1, -1
    };
    FaceMotionModel motion = {0, 0, 0u};
    const int32_t energy = aperture > 35u ? (int32_t)aperture - 35 : 0;
    const uint32_t wave_index = (now_ms / 45u) % 16u;
    motion.shake_x = (int16_t)((wave[wave_index] * energy) / 65);
    motion.shake_y = (int16_t)((wave[(wave_index + 4u) % 16u] * energy) / 130);

    uint32_t blink_phase;
    if (mood == FACE_LINK_LOST) {
        blink_phase = now_ms % 500u;
        motion.blink_closure = blink_phase <= 250u ?
            (uint8_t)((blink_phase * 100u) / 250u) :
            (uint8_t)(((500u - blink_phase) * 100u) / 250u);
    } else {
        blink_phase = now_ms % 4400u;
        if (blink_phase >= 4200u && blink_phase <= 4320u) {
            motion.blink_closure = blink_phase <= 4260u ?
                (uint8_t)(((blink_phase - 4200u) * 100u) / 60u) :
                (uint8_t)(((4320u - blink_phase) * 100u) / 60u);
        }
    }
    return motion;
}

FaceModel face_model_from_state(const VehicleState *state, bool low_battery)
{
    FaceModel model = {FACE_IDLE, 0, 35u};
    if (state == 0) {
        return model;
    }

    const float steering = clamp_unit(state->steering);
    const float throttle = clamp_unit(state->throttle);
    const float magnitude = throttle < 0.0f ? -throttle : throttle;
    model.gaze_x = (int16_t)(steering * 24.0f);
    model.aperture = (uint8_t)(35.0f + magnitude * 65.0f + 0.5f);

    if (state->link == LINK_LOST) {
        model.mood = FACE_LINK_LOST;
    } else if (low_battery) {
        model.mood = FACE_LOW_BATTERY;
    } else if (throttle <= FACE_REVERSE_THRESHOLD) {
        model.mood = FACE_REVERSE;
    } else if (magnitude >= FACE_FOCUSED_THRESHOLD) {
        model.mood = FACE_FOCUSED;
    }
    return model;
}
