#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "diagnostics.h"
#include "vehicle_state.h"

bool f469_ui_init(void);
void f469_ui_tick(uint32_t now_ms, const VehicleState *state,
                  const DiagnosticsSnapshot *diagnostics,
                  bool low_battery, bool demo);
void f469_ui_button(bool touch_available);
void f469_ui_swipe(bool left, uint32_t now_ms);
bool f469_ui_brightness_hit_test(int32_t x, int32_t y);
