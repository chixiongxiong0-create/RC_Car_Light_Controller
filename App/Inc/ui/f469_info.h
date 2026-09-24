#pragma once

#include <stdint.h>

#include "diagnostics.h"
#include "lvgl.h"
#include "ui/two_screen_controller.h"

lv_obj_t *f469_info_create(lv_obj_t *parent);
void f469_info_update(uint32_t now_ms, const VehicleState *state,
                       const DiagnosticsSnapshot *diagnostics, InfoPage page);
