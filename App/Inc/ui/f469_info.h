#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "diagnostics.h"
#include "lvgl.h"
#include "ui/two_screen_controller.h"

lv_obj_t *f469_info_create(lv_obj_t *parent);
bool f469_info_brightness_hit_test(int32_t x, int32_t y);
void f469_info_update(uint32_t now_ms, const VehicleState *state,
                       const DiagnosticsSnapshot *diagnostics, InfoPage page);
