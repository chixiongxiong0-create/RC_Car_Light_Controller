#pragma once

#include <stdint.h>
#include "vehicle_state.h"

typedef struct {
    char battery[16], throttle[16], steering[16], motion[16];
    char pitch[16], roll[16], gps[16], armed[16], link[16];
} OverviewModel;

void overview_model_from_state(const VehicleState *state, uint32_t now_ms,
                               OverviewModel *out);
