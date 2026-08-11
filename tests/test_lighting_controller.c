#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "lighting/lighting_controller.h"

static VehicleState lighting_state(void)
{
    VehicleState state = {0};
    state.aux6 = -1.0f;
    state.aux7 = -1.0f;
    state.aux8 = -1.0f;
    state.aux9 = -1.0f;
    state.link = LINK_OK;
    return state;
}

static void assert_rgb(LedRgb actual, LedRgb expected)
{
    assert(actual.r == expected.r);
    assert(actual.g == expected.g);
    assert(actual.b == expected.b);
}

static void assert_frame_off(const LightingFrame *frame)
{
    assert(frame->front_duty == 0u);
    assert(frame->roof_spot_duty == 0u);
    assert(frame->roof_mode == ROOF_LIGHT_OFF);
    for (size_t i = 0u; i < LED_MAX_PIXELS; ++i) {
        assert_rgb(frame->pixels[i], (LedRgb){0u, 0u, 0u});
    }
}

static void test_fail_safe_duties_and_bounds(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    assert(controller.roof_mode == ROOF_LIGHT_OFF);

    state.aux6 = -1.0f;
    state.aux7 = 1.0f;
    lighting_controller_render(&controller, 0u, &state, false, false,
                               &frame, 4u);
    assert(frame.front_duty == 0u);
    assert(frame.roof_spot_duty == 1000u);
    for (size_t i = 0u; i < 4u; ++i) {
        assert_rgb(frame.pixels[i], (LedRgb){12u, 0u, 0u});
    }
    for (size_t i = 4u; i < LED_MAX_PIXELS; ++i) {
        assert_rgb(frame.pixels[i], (LedRgb){0u, 0u, 0u});
    }

    state.aux6 = 0.0f;
    state.aux7 = 0.0f;
    lighting_controller_render(&controller, 1u, &state, false, false,
                               &frame, 4u);
    assert(frame.front_duty == 500u);
    assert(frame.roof_spot_duty == 500u);

    state.aux6 = 2.0f;
    state.aux7 = -2.0f;
    lighting_controller_render(&controller, 2u, &state, false, false,
                               &frame, 4u);
    assert(frame.front_duty == 1000u);
    assert(frame.roof_spot_duty == 0u);

    const LinkState invalid_links[] = {
        LINK_STARTING, LINK_STALE, LINK_LOST
    };
    for (size_t i = 0u; i < sizeof invalid_links / sizeof invalid_links[0]; ++i) {
        memset(&frame, 0xA5, sizeof frame);
        state.link = invalid_links[i];
        lighting_controller_render(&controller, 10u, &state, false, true,
                                   &frame, LED_MAX_PIXELS);
        assert_frame_off(&frame);
    }

    state.link = LINK_OK;
    memset(&frame, 0xA5, sizeof frame);
    lighting_controller_render(&controller, 10u, NULL, false, false,
                               &frame, LED_MAX_PIXELS);
    assert_frame_off(&frame);

    memset(&frame, 0xA5, sizeof frame);
    lighting_controller_render(NULL, 10u, &state, false, false,
                               &frame, LED_MAX_PIXELS);
    assert_frame_off(&frame);

    memset(&frame, 0xA5, sizeof frame);
    lighting_controller_render(&controller, 10u, &state, false, false,
                               &frame, 3u);
    assert_frame_off(&frame);

    lighting_controller_render(&controller, 10u, &state, false, false,
                               NULL, LED_MAX_PIXELS);
    lighting_controller_render(NULL, 10u, NULL, false, false, NULL, 0u);

    struct {
        uint32_t before;
        LightingFrame frame;
        uint32_t after;
    } guarded = {0x12345678u, {0}, 0x89ABCDEFu};
    state.aux8 = -1.0f;
    lighting_controller_render(&controller, 20u, &state, false, false,
                               &guarded.frame, LED_MAX_PIXELS + 9u);
    assert(guarded.before == 0x12345678u);
    assert(guarded.after == 0x89ABCDEFu);
    for (size_t i = 0u; i < 4u; ++i) {
        assert_rgb(guarded.frame.pixels[i], (LedRgb){12u, 0u, 0u});
    }
    for (size_t i = 4u; i < LED_MAX_PIXELS; ++i) {
        assert_rgb(guarded.frame.pixels[i], (LedRgb){0u, 0u, 0u});
    }
}

static void assert_decorative_ceiling(const LightingFrame *frame,
                                      size_t pixel_count)
{
    for (size_t i = 4u; i < pixel_count; ++i) {
        assert(frame->pixels[i].r <= 96u);
        assert(frame->pixels[i].g <= 96u);
        assert(frame->pixels[i].b <= 96u);
    }
}

static void select_mode(LightingController *controller, LightingFrame *frame,
                        VehicleState *state, float aux8, float aux9,
                        uint32_t now_ms, size_t count)
{
    state->aux8 = aux8;
    state->aux9 = aux9;
    lighting_controller_render(controller, now_ms, state, false, false,
                               frame, count);
}

static void test_roof_modes_hysteresis_and_animation(void)
{
    static const struct {
        float aux8;
        RoofLightMode mode;
    } bands[] = {
        {-0.875f, ROOF_LIGHT_OFF},
        {-0.625f, ROOF_LIGHT_STEADY_WHITE},
        {-0.375f, ROOF_LIGHT_WARM_TRAIL},
        {-0.125f, ROOF_LIGHT_BREATHE_AMBER},
        { 0.125f, ROOF_LIGHT_COMET},
        { 0.375f, ROOF_LIGHT_RAINBOW},
        { 0.625f, ROOF_LIGHT_POLICE},
        { 0.875f, ROOF_LIGHT_STATUS},
    };
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    for (size_t i = 0u; i < sizeof bands / sizeof bands[0]; ++i) {
        lighting_controller_init(&controller);
        select_mode(&controller, &frame, &state, bands[i].aux8, 1.0f,
                    125u, 8u);
        assert(frame.roof_mode == bands[i].mode);
        assert(controller.roof_mode == bands[i].mode);
        for (size_t rear = 0u; rear < 4u; ++rear) {
            assert_rgb(frame.pixels[rear], (LedRgb){12u, 0u, 0u});
        }
        assert_decorative_ceiling(&frame, 8u);
    }

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.625f, 1.0f, 0u, 8u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);
    select_mode(&controller, &frame, &state, -0.490f, 1.0f, 0u, 8u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);
    select_mode(&controller, &frame, &state, -0.460f, 1.0f, 0u, 8u);
    assert(frame.roof_mode == ROOF_LIGHT_WARM_TRAIL);
    select_mode(&controller, &frame, &state, -0.510f, 1.0f, 0u, 8u);
    assert(frame.roof_mode == ROOF_LIGHT_WARM_TRAIL);
    select_mode(&controller, &frame, &state, -0.540f, 1.0f, 0u, 8u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.625f, -1.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){0u, 0u, 0u});
    select_mode(&controller, &frame, &state, -0.625f, 0.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){48u, 48u, 48u});
    select_mode(&controller, &frame, &state, -0.625f, 1.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){96u, 96u, 96u});

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.375f, 0.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){48u, 24u, 4u});

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.125f, 1.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){0u, 0u, 0u});
    select_mode(&controller, &frame, &state, -0.125f, 1.0f, 100u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){96u, 32u, 0u});
    select_mode(&controller, &frame, &state, -0.125f, -1.0f, 100u, 8u);
    assert(frame.pixels[4].r < 96u);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.125f, 1.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){96u, 48u, 8u});
    select_mode(&controller, &frame, &state, 0.125f, 1.0f, 100u, 8u);
    assert_rgb(frame.pixels[5], (LedRgb){96u, 48u, 8u});

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.375f, 1.0f, 0u, 8u);
    const LedRgb rainbow_a = frame.pixels[4];
    select_mode(&controller, &frame, &state, 0.375f, 1.0f, 100u, 8u);
    const LedRgb rainbow_b = frame.pixels[4];
    assert(memcmp(&rainbow_a, &rainbow_b, sizeof rainbow_a) != 0);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.625f, 1.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){96u, 0u, 0u});
    assert_rgb(frame.pixels[5], (LedRgb){0u, 0u, 96u});
    select_mode(&controller, &frame, &state, 0.625f, 1.0f, 200u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){0u, 0u, 96u});
    assert_rgb(frame.pixels[5], (LedRgb){96u, 0u, 0u});

    lighting_controller_init(&controller);
    state.battery_valid = true;
    state.battery_v = 8.0f;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 0u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){0u, 48u, 0u});
    state.battery_v = 6.5f;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 100u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){48u, 16u, 0u});
    state.battery_valid = false;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 200u, 8u);
    assert_rgb(frame.pixels[4], (LedRgb){0u, 0u, 48u});

    assert_decorative_ceiling(&frame, 8u);
}

static void assert_rear(const LightingFrame *frame,
                        LedRgb p0, LedRgb p1, LedRgb p2, LedRgb p3)
{
    assert_rgb(frame->pixels[0], p0);
    assert_rgb(frame->pixels[1], p1);
    assert_rgb(frame->pixels[2], p2);
    assert_rgb(frame->pixels[3], p3);
}

static void render_rear(LightingController *controller, LightingFrame *frame,
                        VehicleState *state, uint32_t now_ms)
{
    lighting_controller_render(controller, now_ms, state, false, false,
                               frame, 4u);
}

static void test_brake_reverse_and_rearm(void)
{
    const LedRgb base = {12u, 0u, 0u};
    const LedRgb brake = {96u, 0u, 0u};
    const LedRgb reverse = {96u, 96u, 96u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 100u);
    assert_rear(&frame, base, base, base, base);
    state.throttle = 0.24f;
    render_rear(&controller, &frame, &state, 110u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 710u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 711u);
    assert_rear(&frame, base, base, base, base);

    state.throttle = 0.24f;
    render_rear(&controller, &frame, &state, 720u);
    state.throttle = 0.05f;
    render_rear(&controller, &frame, &state, 730u);
    assert_rear(&frame, base, base, base, base);

    state.throttle = 0.30f;
    render_rear(&controller, &frame, &state, 800u);
    state.throttle = 0.14f;
    render_rear(&controller, &frame, &state, 810u);
    assert_rear(&frame, brake, brake, brake, brake);

    lighting_controller_init(&controller);
    state.throttle = 0.30f;
    render_rear(&controller, &frame, &state, 900u);
    state.throttle = 0.20f;
    render_rear(&controller, &frame, &state, 910u);
    assert_rear(&frame, base, base, base, base);
    state.throttle = 0.10f;
    render_rear(&controller, &frame, &state, 920u);
    assert_rear(&frame, brake, brake, brake, brake);

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, UINT32_MAX - 255u);
    state.throttle = 0.10f;
    render_rear(&controller, &frame, &state, UINT32_MAX - 15u);
    render_rear(&controller, &frame, &state, 584u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 585u);
    assert_rear(&frame, base, base, base, base);

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 1000u);
    state.throttle = -0.50f;
    render_rear(&controller, &frame, &state, 1010u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 1610u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 1611u);
    assert_rear(&frame, base, reverse, reverse, base);

    lighting_controller_init(&controller);
    state.throttle = -0.50f;
    render_rear(&controller, &frame, &state, 0u);
    assert_rear(&frame, base, reverse, reverse, base);
}

static void test_turn_and_reverse_turn(void)
{
    const LedRgb base = {12u, 0u, 0u};
    const LedRgb reverse = {96u, 96u, 96u};
    const LedRgb amber = {96u, 32u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 0u);
    assert_rear(&frame, amber, amber, base, base);
    render_rear(&controller, &frame, &state, 332u);
    assert_rear(&frame, amber, amber, base, base);
    render_rear(&controller, &frame, &state, 333u);
    assert_rear(&frame, base, base, base, base);

    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 666u);
    assert_rear(&frame, base, base, amber, amber);

    state.throttle = -0.50f;
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 666u);
    assert_rear(&frame, amber, reverse, reverse, base);
    render_rear(&controller, &frame, &state, 999u);
    assert_rear(&frame, base, reverse, reverse, base);

    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 1332u);
    assert_rear(&frame, base, reverse, reverse, amber);
}

static void assert_roof_off(const LightingFrame *frame, size_t count)
{
    for (size_t i = 4u; i < count; ++i) {
        assert_rgb(frame->pixels[i], (LedRgb){0u, 0u, 0u});
    }
}

static void test_warning_pulses_and_priority(void)
{
    const LedRgb black = {0u, 0u, 0u};
    const LedRgb board_red = {64u, 0u, 0u};
    const LedRgb battery_orange = {64u, 8u, 0u};
    const LedRgb brake = {96u, 0u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    state.aux8 = -0.625f;
    state.aux9 = 1.0f;

    lighting_controller_init(&controller);
    const uint32_t board_on[] = {0u, 200u, 400u};
    for (size_t i = 0u; i < sizeof board_on / sizeof board_on[0]; ++i) {
        lighting_controller_render(&controller, board_on[i], &state,
                                   false, true, &frame, 8u);
        assert_rear(&frame, board_red, board_red, board_red, board_red);
        assert_roof_off(&frame, 8u);
    }
    lighting_controller_render(&controller, 100u, &state, false, true,
                               &frame, 8u);
    assert_rear(&frame, black, black, black, black);
    lighting_controller_render(&controller, 500u, &state, false, true,
                               &frame, 8u);
    assert_rear(&frame, black, black, black, black);

    lighting_controller_init(&controller);
    const uint32_t battery_on[] = {0u, 400u, 800u};
    for (size_t i = 0u; i < sizeof battery_on / sizeof battery_on[0]; ++i) {
        lighting_controller_render(&controller, battery_on[i], &state,
                                   true, false, &frame, 8u);
        assert_rear(&frame, battery_orange, battery_orange,
                    battery_orange, battery_orange);
        assert_roof_off(&frame, 8u);
    }
    lighting_controller_render(&controller, 200u, &state, true, false,
                               &frame, 8u);
    assert_rear(&frame, black, black, black, black);
    lighting_controller_render(&controller, 1000u, &state, true, false,
                               &frame, 8u);
    assert_rear(&frame, black, black, black, black);

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    lighting_controller_render(&controller, 390u, &state, false, false,
                               &frame, 8u);
    state.throttle = -0.50f;
    state.steering = -0.50f;
    lighting_controller_render(&controller, 400u, &state, true, true,
                               &frame, 8u);
    assert_rear(&frame, board_red, board_red, board_red, board_red);
    assert_roof_off(&frame, 8u);

    lighting_controller_render(&controller, 400u, &state, true, false,
                               &frame, 8u);
    assert_rear(&frame, battery_orange, battery_orange,
                battery_orange, battery_orange);
    assert_roof_off(&frame, 8u);

    lighting_controller_render(&controller, 400u, &state, false, false,
                               &frame, 8u);
    assert_rear(&frame, brake, brake, brake, brake);
    assert_rgb(frame.pixels[4], (LedRgb){96u, 96u, 96u});

    lighting_controller_render(&controller, 700u, &state, true, true,
                               &frame, 8u);
    assert_rear(&frame, black, black, black, black);
    assert_roof_off(&frame, 8u);

    state.link = LINK_LOST;
    lighting_controller_render(&controller, 400u, &state, true, true,
                               &frame, 8u);
    assert_frame_off(&frame);
}

void test_lighting_controller(void)
{
    test_fail_safe_duties_and_bounds();
    test_roof_modes_hysteresis_and_animation();
    test_brake_reverse_and_rearm();
    test_turn_and_reverse_turn();
    test_warning_pulses_and_priority();
}
