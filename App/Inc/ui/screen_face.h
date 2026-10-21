#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vehicle_state.h"

typedef struct _lv_obj_t lv_obj_t;

typedef enum {
    FACE_IDLE,
    FACE_FOCUSED,
    FACE_RAGE,
    FACE_TURN,
    FACE_BRAKE,
    FACE_REVERSE,
    FACE_HAPPY,
    FACE_LOW_BATTERY,
    FACE_LINK_LOST,
    FACE_SLEEPY,
    FACE_CONFUSED,
    FACE_ANGRY,
    FACE_TAUNT,
    FACE_WINK_LEFT,
    FACE_WINK_RIGHT,
    FACE_SURPRISED,
    FACE_MOOD_COUNT
} FaceMood;

typedef enum {
    FACE_ACTION_NONE,
    FACE_ACTION_BLINK,
    FACE_ACTION_DOUBLE_BLINK,
    FACE_ACTION_SMILE,
    FACE_ACTION_TONGUE,
    FACE_ACTION_WINK_LEFT,
    FACE_ACTION_WINK_RIGHT,
    FACE_ACTION_SCAN,
    FACE_ACTION_BROW,
    FACE_ACTION_SMIRK,
    FACE_ACTION_COUNT
} FaceAction;

typedef struct {
    uint32_t primary_rgb;
    uint32_t secondary_rgb;
    uint32_t background_rgb;
    uint32_t frame_rgb;
    uint32_t eye_rgb;
} FaceTheme;

typedef struct {
    FaceMood mood;
    int16_t gaze_x;
    uint8_t aperture;
} FaceModel;

typedef struct {
    uint8_t percent;
    uint8_t segments;
    bool valid;
    bool low;
} FaceBatteryModel;

typedef struct {
    int16_t shake_x;
    int16_t shake_y;
    int16_t gaze_offset_x;
    int16_t pupil_offset_y;
    uint8_t blink_closure;
} FaceMotionModel;

#define FACE_REVERSE_THRESHOLD       (-0.05f)
#define FACE_FOCUSED_THRESHOLD       (0.35f)

FaceModel face_model_from_state(const VehicleState *state, bool low_battery);
FaceBatteryModel face_battery_model(float battery_v, bool voltage_valid,
                                    uint8_t cell_count);
FaceMotionModel face_motion_model(uint32_t now_ms, uint8_t aperture,
                                  FaceMood mood);
FaceTheme face_theme(FaceMood mood);
const char *face_mood_name(FaceMood mood);
bool face_mood_is_alert(FaceMood mood);
lv_obj_t *screen_face_create(void);
void screen_face_update(uint32_t now_ms, const VehicleState *state, bool low_battery);
