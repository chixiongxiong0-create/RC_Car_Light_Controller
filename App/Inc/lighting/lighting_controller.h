#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "led/led_controller.h"
#include "vehicle_state.h"

typedef enum {
    ROOF_LIGHT_OFF,
    ROOF_LIGHT_STEADY_WHITE,
    ROOF_LIGHT_WARM_TRAIL,
    ROOF_LIGHT_BREATHE_AMBER,
    ROOF_LIGHT_COMET,
    ROOF_LIGHT_RAINBOW,
    ROOF_LIGHT_POLICE,
    ROOF_LIGHT_STATUS,
    ROOF_LIGHT_MODE_COUNT
} RoofLightMode;

typedef struct {
    uint16_t front_duty;
    uint16_t roof_spot_duty;
    uint32_t estimated_ma;
    RoofLightMode roof_mode;
    LedRgb pixels[LED_MAX_PIXELS];
} LightingFrame;

typedef struct {
    RoofLightMode roof_mode;
    float previous_throttle;
    uint32_t brake_trigger_ms;
    bool has_previous_throttle;
    bool forward_armed;
    bool brake_active;
} LightingController;

void lighting_controller_init(LightingController *controller);
void lighting_controller_render(LightingController *controller,
                                uint32_t now_ms,
                                const VehicleState *state,
                                bool low_battery,
                                bool board_fault,
                                LightingFrame *frame,
                                size_t pixel_count);
