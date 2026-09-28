#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "ui/screen_face.h"

lv_obj_t *f469_face_create(lv_obj_t *parent);
void f469_face_update(uint32_t now_ms, const VehicleState *state,
                       FaceMood mood, bool low_battery, bool manual);
