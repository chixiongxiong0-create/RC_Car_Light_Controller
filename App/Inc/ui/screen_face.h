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

#define FACE_REVERSE_THRESHOLD       (-0.05f)
#define FACE_FOCUSED_THRESHOLD       (0.35f)

FaceModel face_model_from_state(const VehicleState *state, bool low_battery);
lv_obj_t *screen_face_create(void);
void screen_face_update(uint32_t now_ms, const VehicleState *state, bool low_battery);
