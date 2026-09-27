#pragma once

#include <stdint.h>

#include "lighting/lighting_controller.h"

void f469_vehicle_effects_render(LightingController *controller,
                                 uint32_t now_ms, const VehicleState *state,
                                 Ws2812Frame *frame);
