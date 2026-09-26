#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "lighting/lighting_controller.h"
#include "vehicle_state.h"

static MspFrame rc_frame(uint16_t throttle, uint16_t steering,
                         uint16_t lamps, uint16_t mode, uint16_t effect)
{
    MspFrame frame = {0};
    frame.command = 105u;
    frame.length = 24u;
    const uint16_t channels[12] = {
        1000u, throttle, steering, 2000u, 1000u, 1000u,
        1000u, lamps, 1000u, 1000u, mode, effect
    };
    for (unsigned i = 0; i < 12u; ++i) {
        frame.payload[2u * i] = (uint8_t)channels[i];
        frame.payload[2u * i + 1u] = (uint8_t)(channels[i] >> 8);
    }
    return frame;
}

static void near(float actual, float expected)
{
    const float difference = actual - expected;
    assert(difference > -0.001f && difference < 0.001f);
}

int main(void)
{
    vehicle_state_init();
    MspFrame input = rc_frame(2000u, 1000u, 1500u, 2000u, 1750u);
    assert(vehicle_state_on_msp(&input, 1u));
    const VehicleState *state = vehicle_state_get();
    near(state->throttle, 1.0f);
    near(state->steering, -1.0f);
    near(state->aux4, 0.0f);
    near(state->aux7, 1.0f);
    near(state->aux8, 0.5f);
    assert(state->lighting_rc_valid);

    LightingController controller;
    LightingFrame lights;
    lighting_controller_init(&controller);
    lighting_controller_render(&controller, 1u, state, false, false, &lights);
    assert(lights.front_duty == 1000u);
    assert(lights.roof_spot_duty == 0u);
    assert(lights.roof_mode != ROOF_LIGHT_OFF);

    input = rc_frame(2000u, 1000u, 2000u, 1000u, 1750u);
    assert(vehicle_state_on_msp(&input, 2u));
    lighting_controller_render(&controller, 2u, vehicle_state_get(),
                               false, false, &lights);
    assert(lights.front_duty == 1000u);
    assert(lights.roof_spot_duty == 1000u);
    assert(lights.roof_mode == ROOF_LIGHT_OFF);

    input.length = 22u;
    assert(vehicle_state_on_msp(&input, 3u));
    assert(!vehicle_state_get()->lighting_rc_valid);
    lighting_controller_render(&controller, 3u, vehicle_state_get(),
                               false, false, &lights);
    assert(lights.front_duty == 0u);
    assert(lights.roof_spot_duty == 0u);
    return 0;
}
