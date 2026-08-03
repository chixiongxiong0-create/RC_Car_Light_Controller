#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "vehicle_state.h"

typedef struct {
    bool real_seen;
    bool blending;
    uint32_t blend_started_ms;
    VehicleState demo;
    VehicleState blend_from;
    VehicleState selected;
} VehicleStateSource;

void vehicle_state_source_init(VehicleStateSource *source);
void vehicle_state_source_note_real(VehicleStateSource *source, uint32_t now_ms);
void vehicle_state_source_tick(VehicleStateSource *source, uint32_t now_ms,
                               uint8_t battery_cells,
                               const VehicleState *real_state);
const VehicleState *vehicle_state_source_get(const VehicleStateSource *source);
bool vehicle_state_source_is_demo(const VehicleStateSource *source);
