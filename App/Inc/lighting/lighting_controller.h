#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "led/ws2812_frame.h"
#include "vehicle_state.h"

#define LIGHTING_ESTIMATED_CURRENT_BUDGET_MA 850u

/* Logical pixel 0 is the front of the car. F469 physical side wiring was
 * verified on the vehicle: group 3 is left, group 2 is right, and the left
 * chain's DIN-to-DOUT direction is reversed. */
#ifdef F469_RC_LAYOUT
#define LIGHTING_LEFT_STRIP_GROUP 3u
#define LIGHTING_RIGHT_STRIP_GROUP 2u
#define LIGHTING_LEFT_STRIP_REVERSED 1u
#define LIGHTING_RIGHT_STRIP_REVERSED 0u
#else
#define LIGHTING_LEFT_STRIP_GROUP 2u
#define LIGHTING_RIGHT_STRIP_GROUP 3u
#define LIGHTING_LEFT_STRIP_REVERSED 0u
#define LIGHTING_RIGHT_STRIP_REVERSED 0u
#endif

typedef enum {
    ROOF_LIGHT_OFF,
    ROOF_LIGHT_STEADY_WHITE,
    ROOF_LIGHT_WARM_TRAIL,
    ROOF_LIGHT_BREATHE_AMBER,
    ROOF_LIGHT_COMET,
    ROOF_LIGHT_RAINBOW,
    ROOF_LIGHT_DRIVE_SYNC,
    ROOF_LIGHT_STATUS,
    ROOF_LIGHT_MODE_COUNT
} RoofLightMode;

typedef struct {
    uint16_t front_duty;
    uint16_t roof_spot_duty;
    uint32_t estimated_ma;
    RoofLightMode roof_mode;
    Ws2812Frame ws2812;
} LightingFrame;

typedef struct {
    RoofLightMode roof_mode;
    float previous_throttle;
    uint32_t brake_trigger_ms;
    uint32_t turn_start_ms;
    uint32_t color_phase_milli;
    uint32_t color_last_ms;
    int8_t active_turn;
    bool color_clock_started;
    bool has_previous_throttle;
    bool forward_armed;
    bool brake_active;
    bool has_seen_valid_lighting_rc;
} LightingController;

void lighting_controller_init(LightingController *controller);
void lighting_controller_render(LightingController *controller,
                                uint32_t now_ms,
                                const VehicleState *state,
                                bool low_battery,
                                bool board_fault,
                                LightingFrame *frame);
