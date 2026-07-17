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
