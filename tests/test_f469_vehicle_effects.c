#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "lighting/lighting_controller.h"

static VehicleState car(float throttle, float steering)
{
    VehicleState state = {0};
    state.link = LINK_OK;
    state.lighting_rc_valid = true;
    state.throttle = throttle;
    state.steering = steering;
    state.aux4 = -1.0f;
    state.aux7 = -1.0f;
    state.aux8 = -1.0f;
    return state;
}

static LightingFrame render(LightingController *controller, uint32_t ms,
                            const VehicleState *state)
{
    LightingFrame frame;
    lighting_controller_render(controller, ms, state, false, false, &frame);
    assert(frame.estimated_ma <= LIGHTING_ESTIMATED_CURRENT_BUDGET_MA);
    return frame;
}

static bool same(LedRgb a, LedRgb b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b;
}

static unsigned energy(LedRgb pixel)
{
    return pixel.r + pixel.g + pixel.b;
}

static LedRgb side_pixel(const LightingFrame *frame, unsigned side, unsigned index)
{
    const unsigned group = side == 0u ? LIGHTING_LEFT_STRIP_GROUP :
                                        LIGHTING_RIGHT_STRIP_GROUP;
    const unsigned reversed = side == 0u ? LIGHTING_LEFT_STRIP_REVERSED :
                                           LIGHTING_RIGHT_STRIP_REVERSED;
    return frame->ws2812.groups[group][reversed ? 7u - index : index];
}

static void test_stopped_rainbow_breathes_with_spatial_phase(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(0.0f, 0.0f);
    const LightingFrame first = render(&controller, 0u, &state);
    const LightingFrame later = render(&controller, 1250u, &state);
    const LightingFrame repeat = render(&controller, 5000u, &state);
    assert(!same(first.ws2812.groups[0][0], first.ws2812.groups[0][1]));
    assert(!same(side_pixel(&first, 0u, 0u), side_pixel(&first, 0u, 1u)));
    assert(!same(first.ws2812.groups[0][0], later.ws2812.groups[0][0]));
    assert(same(first.ws2812.groups[0][0], repeat.ws2812.groups[0][0]));
    assert(same(side_pixel(&first, 0u, 0u), side_pixel(&repeat, 0u, 0u)));
}

static void test_forward_palette_moves_at_throttle_rate(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(0.5f, 0.0f);
    const LightingFrame start = render(&controller, 0u, &state);
    const LightingFrame before_step = render(&controller, 50u, &state);
    const LightingFrame next_step = render(&controller, 60u, &state);
    assert(same(start.ws2812.groups[0][0], before_step.ws2812.groups[0][0]));
    assert(!same(start.ws2812.groups[0][0], next_step.ws2812.groups[0][0]));
    assert(same(start.ws2812.groups[0][1], next_step.ws2812.groups[0][0]));
    assert(energy(side_pixel(&start, 0u, 0u)) > energy(side_pixel(&start, 0u, 7u)));
    assert(energy(side_pixel(&start, 1u, 0u)) > energy(side_pixel(&start, 1u, 7u)));
    assert(!same(side_pixel(&start, 0u, 0u), side_pixel(&start, 0u, 1u)));
}

static void test_reverse_is_white_dominant_and_moves_backward(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(-0.5f, 0.0f);
    const LightingFrame start = render(&controller, 0u, &state);
    const LightingFrame next = render(&controller, 60u, &state);
    const LedRgb rear = start.ws2812.groups[0][0];
    assert(rear.r > 0u && rear.r == rear.g && rear.g == rear.b);
    const LedRgb side = side_pixel(&start, 0u, 7u);
    assert(side.r > 0u && side.g > 0u && side.b > 0u);
    assert(energy(side_pixel(&start, 0u, 7u)) > energy(side_pixel(&start, 0u, 0u)));
    assert(!same(side_pixel(&start, 0u, 0u), side_pixel(&next, 0u, 0u)));
}

static void test_both_chains_move_three_times_faster(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(0.5f, 0.0f);
    const LightingFrame start = render(&controller, 0u, &state);
    const LightingFrame before = render(&controller, 50u, &state);
    const LightingFrame after = render(&controller, 60u, &state);
    assert(same(start.ws2812.groups[0][0], before.ws2812.groups[0][0]));
    assert(!same(start.ws2812.groups[0][0], after.ws2812.groups[0][0]));
    assert(same(side_pixel(&start, 0u, 0u), side_pixel(&before, 0u, 0u)));
    assert(!same(side_pixel(&start, 0u, 0u), side_pixel(&after, 0u, 0u)));
}

static void test_throttle_change_keeps_animation_phase(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(0.2f, 0.0f);
    (void)render(&controller, 0u, &state);
    const LightingFrame before = render(&controller, 100u, &state);
    state.throttle = 1.0f;
    const LightingFrame changed = render(&controller, 101u, &state);
    assert(same(before.ws2812.groups[0][0], changed.ws2812.groups[0][0]));
}

static void test_brake_and_turn_take_priority_per_side(void)
{
    LightingController controller;
    lighting_controller_init(&controller);
    VehicleState state = car(0.7f, 0.0f);
    (void)render(&controller, 1000u, &state);
    state.throttle = 0.0f;
    const LightingFrame brake = render(&controller, 1010u, &state);
    const LightingFrame breathe = render(&controller, 1110u, &state);
    for (unsigned side = 0; side < 2u; ++side) {
        assert(brake.ws2812.groups[side][0].r > 0u);
        assert(brake.ws2812.groups[side][0].g == 0u);
        assert(brake.ws2812.groups[side][0].b == 0u);
        assert(side_pixel(&brake, side, 0u).g == 0u);
    }
    assert(!same(side_pixel(&brake, 0u, 0u), side_pixel(&breathe, 0u, 0u)));

    lighting_controller_init(&controller);
    state = car(0.5f, -0.6f);
    const LightingFrame turn_on = render(&controller, 2000u, &state);
    const LightingFrame turn_off = render(&controller, 2340u, &state);
    assert(turn_on.ws2812.groups[0][0].r > 0u);
    assert(turn_on.ws2812.groups[0][0].g > 0u);
    assert(turn_on.ws2812.groups[0][0].b == 0u);
    assert(side_pixel(&turn_on, 0u, 0u).r > 0u);
    assert(side_pixel(&turn_on, 0u, 0u).g > 0u);
    assert(!same(turn_on.ws2812.groups[0][0], turn_off.ws2812.groups[0][0]));
    assert(!same(turn_on.ws2812.groups[1][0], turn_on.ws2812.groups[0][0]));
    assert(!same(side_pixel(&turn_on, 1u, 0u), side_pixel(&turn_on, 0u, 0u)));
}

static void test_ch11_ch12_do_not_change_new_effects(void)
{
    LightingController first_controller, second_controller;
    lighting_controller_init(&first_controller);
    lighting_controller_init(&second_controller);
    VehicleState first_state = car(0.5f, 0.0f);
    VehicleState second_state = first_state;
    second_state.aux7 = 1.0f;
    second_state.aux8 = 1.0f;
    const LightingFrame first = render(&first_controller, 170u, &first_state);
    const LightingFrame second = render(&second_controller, 170u, &second_state);
    for (unsigned group = 0; group < WS2812_GROUP_COUNT; ++group)
        for (unsigned pixel = 0; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel)
            assert(same(first.ws2812.groups[group][pixel],
                        second.ws2812.groups[group][pixel]));
}

int main(void)
{
    assert(LIGHTING_LEFT_STRIP_GROUP == 3u);
    assert(LIGHTING_RIGHT_STRIP_GROUP == 2u);
    assert(LIGHTING_LEFT_STRIP_REVERSED == 1u);
    assert(LIGHTING_RIGHT_STRIP_REVERSED == 0u);
    test_stopped_rainbow_breathes_with_spatial_phase();
    test_forward_palette_moves_at_throttle_rate();
    test_reverse_is_white_dominant_and_moves_backward();
    test_both_chains_move_three_times_faster();
    test_throttle_change_keeps_animation_phase();
    test_brake_and_turn_take_priority_per_side();
    test_ch11_ch12_do_not_change_new_effects();
    puts("F469 vehicle effects passed");
    return 0;
}
