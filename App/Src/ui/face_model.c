#include "ui/screen_face.h"

static const FaceTheme themes[FACE_MOOD_COUNT] = {
    {0x50D9FFu,0xFFC928u,0x07131Bu,0x24566Au,0x142A33u},
    {0xFFC928u,0xFF9F1Cu,0x161106u,0x66521Au,0x29240Fu},
    {0xFF6B2Cu,0xFFD447u,0x1B0905u,0x7D2C14u,0x30150Du},
    {0x38E6C5u,0x8DD8FFu,0x061916u,0x1F6D62u,0x10312Cu},
    {0xFF3B47u,0xF8FBFFu,0x1B0608u,0x7B2028u,0x301014u},
    {0xBDEFFFu,0xFFFFFFu,0x07131Au,0x4C7482u,0x142730u},
    {0xFF72B6u,0x55E8FFu,0x190817u,0x7E315Cu,0x321126u},
    {0xFF5964u,0xFF9E3Du,0x180709u,0x6E2428u,0x2D1113u},
    {0xFF5964u,0xF5F5F5u,0x17050Du,0x84233Du,0x2E0C18u},
    {0x7F8CFFu,0xC7A7FFu,0x090A1Du,0x3B4175u,0x171831u},
    {0xB66CFFu,0x6FFFD0u,0x13081Du,0x5A2D7Cu,0x251032u},
    {0xFF3A32u,0xFF922Bu,0x1D0503u,0x81241Eu,0x35100Cu},
    {0xED4CFFu,0xFF7BACu,0x18051Cu,0x6C2673u,0x2D1033u},
    {0x4DE1FFu,0xFF72B6u,0x07151Au,0x28697Au,0x102B34u},
    {0xFF72B6u,0xFFC928u,0x190817u,0x7E315Cu,0x321126u},
    {0xA8FF4Fu,0x50D9FFu,0x0D1807u,0x4E7629u,0x1D3111u}
};

static const char *const mood_names[FACE_MOOD_COUNT] = {
    "IDLE", "FOCUSED", "BOOST", "STEER", "BRAKE", "REVERSE",
    "HAPPY", "LOW BAT", "LINK LOST", "SLEEPY", "CONFUSED", "ANGRY",
    "TAUNT", "WINK L", "WINK R", "SURPRISED"
};

FaceTheme face_theme(FaceMood mood)
{
    return themes[(unsigned)mood < FACE_MOOD_COUNT ? (unsigned)mood : 0u];
}

const char *face_mood_name(FaceMood mood)
{
    return mood_names[(unsigned)mood < FACE_MOOD_COUNT ? (unsigned)mood : 0u];
}

bool face_mood_is_alert(FaceMood mood)
{
    return mood == FACE_LOW_BATTERY || mood == FACE_LINK_LOST;
}

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
    FaceMotionModel motion = {0};
    const int32_t energy = aperture > 35u ? (int32_t)aperture - 35 : 0;
    const uint32_t wave_index = (now_ms / 45u) % 16u;
    motion.shake_x = (int16_t)((wave[wave_index] * energy) / 25);
    motion.shake_y = (int16_t)(wave[wave_index] * (mood == FACE_FOCUSED ? 4 : 2));
    motion.gaze_offset_x = (int16_t)(wave[(now_ms / 90u) % 16u] *
                                      (mood == FACE_IDLE ? 8 : 3));
    motion.pupil_offset_y = (int16_t)(wave[(wave_index + 4u) % 16u] * 3);
    if (mood == FACE_REVERSE) {
        motion.shake_x -= 8;
        motion.gaze_offset_x -= 7;
    }

    uint32_t blink_phase;
    if (mood == FACE_LINK_LOST) {
        blink_phase = now_ms % 500u;
        motion.blink_closure = blink_phase <= 250u ?
            (uint8_t)((blink_phase * 100u) / 250u) :
            (uint8_t)(((500u - blink_phase) * 100u) / 250u);
    } else {
        blink_phase = now_ms % 4400u;
        if (blink_phase >= 2620u && blink_phase <= 2740u) {
            motion.blink_closure = blink_phase <= 2680u ?
                (uint8_t)(((blink_phase - 2620u) * 100u) / 60u) :
                (uint8_t)(((2740u - blink_phase) * 100u) / 60u);
        } else if (blink_phase >= 4200u && blink_phase <= 4320u) {
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
    } else if (magnitude >= 0.72f) {
        model.mood = FACE_RAGE;
    } else if (steering <= -0.45f || steering >= 0.45f) {
        model.mood = FACE_TURN;
    } else if (magnitude >= FACE_FOCUSED_THRESHOLD) {
        model.mood = FACE_FOCUSED;
    }
    return model;
}
