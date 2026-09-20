#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vehicle_state.h"

typedef struct _lv_obj_t lv_obj_t;

typedef enum {
    FACE_IDLE,
    FACE_FOCUSED,
    FACE_REVERSE,
    FACE_LOW_BATTERY,
    FACE_LINK_LOST
} FaceMood;

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
    uint8_t blink_closure;
} FaceMotionModel;

#define FACE_REVERSE_THRESHOLD       (-0.05f)
#define FACE_FOCUSED_THRESHOLD       (0.35f)

FaceModel face_model_from_state(const VehicleState *state, bool low_battery);
FaceBatteryModel face_battery_model(float battery_v, bool voltage_valid,
                                    uint8_t cell_count);
FaceMotionModel face_motion_model(uint32_t now_ms, uint8_t aperture,
                                  FaceMood mood);
lv_obj_t *screen_face_create(void);
void screen_face_update(uint32_t now_ms, const VehicleState *state, bool low_battery);
