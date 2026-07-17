#pragma once

#include "lvgl.h"
#include "vehicle_state.h"

lv_obj_t *screen_dashboard_create(void);
void screen_dashboard_update(const VehicleState *state);
