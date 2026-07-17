#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/ui_types.h"
#include "vehicle_state.h"

void input_manager_init(void);
void input_manager_set_button(bool pressed, uint32_t now_ms);
void input_manager_set_touch_available(bool available);
void input_manager_set_touch_page(UiPage page);
void input_manager_tick(uint32_t now_ms, const VehicleState *state);
UiPage input_manager_page(void);
bool input_manager_take_brightness_request(void);
