#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "led/ws2812_frame.h"
#include "lighting/lighting_controller.h"

static VehicleState lighting_state(void)
{
    VehicleState state = {0};
    state.aux6 = -1.0f;
    state.aux7 = -1.0f;
    state.aux8 = -1.0f;
    state.aux9 = -1.0f;
    state.lighting_rc_valid = true;
    state.link = LINK_OK;
    return state;
}

static void assert_rgb(LedRgb actual, LedRgb expected)
{
    assert(actual.r == expected.r);
    assert(actual.g == expected.g);
    assert(actual.b == expected.b);
}

static void assert_groups_black(const Ws2812Frame *frame)
{
    for (size_t group = 0u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            assert_rgb(frame->groups[group][pixel], (LedRgb){0u, 0u, 0u});
        }
    }
}

static uint32_t frame_current(const Ws2812Frame *frame)
{
    LedRgb flat[WS2812_TOTAL_PIXELS];
    size_t offset = 0u;

    for (size_t group = 0u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            flat[offset++] = frame->groups[group][pixel];
        }
    }
    return led_estimated_ma(flat, WS2812_TOTAL_PIXELS);
}

static void assert_current_accounted(const LightingFrame *frame)
{
    assert(frame->estimated_ma == frame_current(&frame->ws2812));
    assert(frame->estimated_ma <= LED_CURRENT_BUDGET_MA);
}

static void test_four_group_contract(void)
{
    assert(WS2812_GROUP_COUNT == 4u);
    assert(WS2812_GROUP_LENGTHS[0] == 4u);
    assert(WS2812_GROUP_LENGTHS[1] == 4u);
    assert(WS2812_GROUP_LENGTHS[2] == 8u);
    assert(WS2812_GROUP_LENGTHS[3] == 8u);
    assert(WS2812_TOTAL_PIXELS == 24u);
}

static void test_startup_frame_is_black_before_valid_lighting_rc(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.lighting_rc_valid = false;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);

    assert(frame.front_duty == 0u);
    assert(frame.roof_spot_duty == 0u);
    assert(frame.estimated_ma == 0u);
    assert(frame.roof_mode == ROOF_LIGHT_OFF);
    assert_groups_black(&frame.ws2812);
}

static void test_rear_groups_are_mirrored_for_left_turn(void)
{
    const LedRgb base = {12u, 0u, 0u};
    const LedRgb amber = {96u, 32u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.steering = -0.50f;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);

    assert_rgb(frame.ws2812.groups[0][0], amber);
    assert_rgb(frame.ws2812.groups[0][1], amber);
    assert_rgb(frame.ws2812.groups[0][2], base);
    assert_rgb(frame.ws2812.groups[0][3], base);
    assert_rgb(frame.ws2812.groups[1][0], base);
    assert_rgb(frame.ws2812.groups[1][1], base);
    assert_rgb(frame.ws2812.groups[1][2], amber);
    assert_rgb(frame.ws2812.groups[1][3], amber);
    assert_current_accounted(&frame);
}

static void test_roof_animation_spans_two_independent_groups(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    bool differs = false;

    lighting_controller_init(&controller);
    state.aux8 = 0.375f;
    state.aux9 = 1.0f;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);

    assert(frame.roof_mode == ROOF_LIGHT_RAINBOW);
    for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[2]; ++pixel) {
        if (memcmp(&frame.ws2812.groups[2][pixel],
                   &frame.ws2812.groups[3][pixel],
                   sizeof(LedRgb)) != 0) {
            differs = true;
        }
    }
    assert(differs);
    assert_current_accounted(&frame);
}

static void test_duties_clamp_and_loss_warning_preserves_roof_black(void)
{
    const LedRgb loss_amber = {32u, 8u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.aux6 = 2.0f;
    state.aux7 = -2.0f;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    assert(frame.front_duty == 1000u);
    assert(frame.roof_spot_duty == 0u);
    assert_current_accounted(&frame);

    state.lighting_rc_valid = false;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    for (size_t pixel = 0u; pixel < 4u; ++pixel) {
        assert_rgb(frame.ws2812.groups[0][pixel], loss_amber);
        assert_rgb(frame.ws2812.groups[1][pixel], loss_amber);
    }
    for (size_t pixel = 0u; pixel < 8u; ++pixel) {
        assert_rgb(frame.ws2812.groups[2][pixel], (LedRgb){0u, 0u, 0u});
        assert_rgb(frame.ws2812.groups[3][pixel], (LedRgb){0u, 0u, 0u});
    }
    assert_current_accounted(&frame);
}

static void test_warning_overrides_both_rear_groups(void)
{
    const LedRgb board_red = {64u, 0u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.aux8 = -0.625f;
    state.aux9 = 1.0f;
    lighting_controller_render(&controller, 0u, &state, false, true, &frame);

    for (size_t group = 0u; group < 2u; ++group) {
        for (size_t pixel = 0u; pixel < 4u; ++pixel) {
            assert_rgb(frame.ws2812.groups[group][pixel], board_red);
        }
    }
    for (size_t pixel = 0u; pixel < 8u; ++pixel) {
        assert_rgb(frame.ws2812.groups[2][pixel], (LedRgb){0u, 0u, 0u});
        assert_rgb(frame.ws2812.groups[3][pixel], (LedRgb){0u, 0u, 0u});
    }
    assert_current_accounted(&frame);
}

void test_lighting_controller(void)
{
    test_four_group_contract();
    test_startup_frame_is_black_before_valid_lighting_rc();
    test_rear_groups_are_mirrored_for_left_turn();
    test_roof_animation_spans_two_independent_groups();
    test_duties_clamp_and_loss_warning_preserves_roof_black();
    test_warning_overrides_both_rear_groups();
}
