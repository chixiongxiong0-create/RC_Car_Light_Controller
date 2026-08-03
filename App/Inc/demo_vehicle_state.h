#pragma once

#include <stdint.h>

#include "vehicle_state.h"

void demo_vehicle_state_sample(uint32_t now_ms, uint8_t battery_cells,
                               VehicleState *out);
