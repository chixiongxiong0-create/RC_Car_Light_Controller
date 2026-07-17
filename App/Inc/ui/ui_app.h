#ifndef UI_APP_H
#define UI_APP_H

#include <stdint.h>

#include "vehicle_state.h"

void ui_app_init(void);
void ui_app_tick(uint32_t now_ms, const VehicleState *state);

#endif
