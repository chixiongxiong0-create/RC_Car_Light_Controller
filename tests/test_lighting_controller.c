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
    assert(frame->estimated_ma <= LIGHTING_ESTIMATED_CURRENT_BUDGET_MA);
}

static LedRgb roof_pixel(const LightingFrame *frame, size_t index)
{
    assert(index < 16u);
    return frame->ws2812.groups[2u + index / 8u][index % 8u];
}

static void assert_rear(const LightingFrame *frame,
                        LedRgb p0, LedRgb p1, LedRgb p2, LedRgb p3)
{
    assert_rgb(frame->ws2812.groups[0][0], p0);
    assert_rgb(frame->ws2812.groups[0][1], p1);
    assert_rgb(frame->ws2812.groups[0][2], p2);
    assert_rgb(frame->ws2812.groups[0][3], p3);
}

static LedRgb drive_test_pixel(const LightingFrame *frame, bool right, size_t logical_index)
{
    const size_t group = right ? LIGHTING_RIGHT_STRIP_GROUP : LIGHTING_LEFT_STRIP_GROUP;
    const bool reversed = right ? LIGHTING_RIGHT_STRIP_REVERSED : LIGHTING_LEFT_STRIP_REVERSED;
    return frame->ws2812.groups[group][reversed ? 7u - logical_index : logical_index];
}

static void assert_rear_side(const LightingFrame *frame, size_t group,
                             LedRgb p0, LedRgb p1, LedRgb p2, LedRgb p3)
{
    assert(group < 2u);
    assert_rgb(frame->ws2812.groups[group][0], p0);
    assert_rgb(frame->ws2812.groups[group][1], p1);
    assert_rgb(frame->ws2812.groups[group][2], p2);
    assert_rgb(frame->ws2812.groups[group][3], p3);
}

static void assert_rear_groups(const LightingFrame *frame, LedRgb color)
{
    for (size_t group = 0u; group < 2u; ++group) {
        for (size_t pixel = 0u; pixel < 4u; ++pixel) {
            assert_rgb(frame->ws2812.groups[group][pixel], color);
        }
    }
}

static void assert_roof_off(const LightingFrame *frame)
{
    for (size_t group = 2u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            assert_rgb(frame->ws2812.groups[group][pixel], (LedRgb){0u, 0u, 0u});
        }
    }
}

static void assert_frame_off(const LightingFrame *frame)
{
    assert(frame->front_duty == 0u);
    assert(frame->roof_spot_duty == 0u);
    assert(frame->estimated_ma == 0u);
    assert(frame->roof_mode == ROOF_LIGHT_OFF);
    assert_groups_black(&frame->ws2812);
}

static void assert_loss_frame(const LightingFrame *frame, LedRgb rear)
{
    assert(frame->front_duty == 0u);
    assert(frame->roof_spot_duty == 0u);
    assert(frame->roof_mode == ROOF_LIGHT_OFF);
    assert_rear_groups(frame, rear);
    assert_roof_off(frame);
    assert_current_accounted(frame);
}

static void render_rear(LightingController *controller, LightingFrame *frame,
                        VehicleState *state, uint32_t now_ms)
{
    lighting_controller_render(controller, now_ms, state, false, false, frame);
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

static void test_fail_safe_duties_and_bounds(void)
{
    LightingController controller;
    LightingController startup_controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    const LinkState invalid_links[] = {LINK_STARTING, LINK_STALE, LINK_LOST};

    lighting_controller_init(&controller);
    assert(controller.roof_mode == ROOF_LIGHT_OFF);
    state.aux6 = -1.0f;
    state.aux7 = 1.0f;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    assert(frame.front_duty == 0u);
    assert(frame.roof_spot_duty == 1000u);
    assert_rear_side(&frame, 0u, (LedRgb){120u, 0u, 0u},
                     (LedRgb){40u, 0u, 0u}, (LedRgb){40u, 0u, 0u},
                     (LedRgb){70u, 0u, 0u});
    assert_roof_off(&frame);
    assert_current_accounted(&frame);

    state.aux6 = 0.0f;
    state.aux7 = 0.0f;
    lighting_controller_render(&controller, 1u, &state, false, false, &frame);
    assert(frame.front_duty == 500u);
    assert(frame.roof_spot_duty == 500u);

    state.aux6 = 2.0f;
    state.aux7 = -2.0f;
    lighting_controller_render(&controller, 2u, &state, false, false, &frame);
    assert(frame.front_duty == 1000u);
    assert(frame.roof_spot_duty == 0u);

    lighting_controller_init(&startup_controller);
    for (size_t i = 0u; i < sizeof invalid_links / sizeof invalid_links[0]; ++i) {
        memset(&frame, 0xA5, sizeof frame);
        state.link = invalid_links[i];
        lighting_controller_render(&startup_controller, 10u, &state, false,
                                   true, &frame);
        assert_frame_off(&frame);
    }

    state.link = LINK_OK;
    memset(&frame, 0xA5, sizeof frame);
    lighting_controller_render(&controller, 10u, NULL, false, false, &frame);
    assert_frame_off(&frame);
    memset(&frame, 0xA5, sizeof frame);
    lighting_controller_render(NULL, 10u, &state, false, false, &frame);
    assert_frame_off(&frame);
    lighting_controller_render(&controller, 10u, &state, false, false, NULL);
    lighting_controller_render(NULL, 10u, NULL, false, false, NULL);

    struct {
        uint32_t before;
        LightingFrame frame;
        uint32_t after;
    } guarded = {0x12345678u, {0}, 0x89ABCDEFu};
    state.aux8 = -1.0f;
    lighting_controller_render(&controller, 20u, &state, false, false,
                               &guarded.frame);
    assert(guarded.before == 0x12345678u);
    assert(guarded.after == 0x89ABCDEFu);
    assert_rear_side(&guarded.frame, 0u, (LedRgb){120u, 0u, 0u},
                     (LedRgb){40u, 0u, 0u}, (LedRgb){40u, 0u, 0u},
                     (LedRgb){70u, 0u, 0u});
    assert_roof_off(&guarded.frame);
    assert_current_accounted(&guarded.frame);
}

static void test_startup_black_and_post_valid_lighting_loss(void)
{
    const LedRgb black = {0u, 0u, 0u};
    const LedRgb loss_amber = {32u, 8u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    state.aux6 = 1.0f;
    state.aux7 = 1.0f;
    state.aux8 = -0.625f;
    state.aux9 = 1.0f;

    lighting_controller_init(&controller);
    state.lighting_rc_valid = false;
    lighting_controller_render(&controller, UINT32_MAX - 50u, &state, false,
                               false, &frame);
    assert_frame_off(&frame);

    state.lighting_rc_valid = true;
    lighting_controller_render(&controller, UINT32_MAX - 40u, &state, false,
                               false, &frame);
    assert(frame.front_duty == 1000u);
    assert(frame.roof_spot_duty == 1000u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){200u, 200u, 200u});
    assert_rgb(roof_pixel(&frame, 8u), (LedRgb){200u, 200u, 200u});

    state.lighting_rc_valid = false;
    state.throttle = -1.0f;
    state.steering = -1.0f;
    const struct {
        uint32_t now_ms;
        LedRgb rear;
    } phases[] = {
        {0u, loss_amber}, {99u, loss_amber}, {100u, black},
        {199u, black}, {200u, loss_amber}, {299u, loss_amber},
        {300u, black}, {1999u, black}, {2000u, loss_amber},
    };
    for (size_t i = 0u; i < sizeof phases / sizeof phases[0]; ++i) {
        lighting_controller_render(&controller, phases[i].now_ms, &state,
                                   true, true, &frame);
        assert_loss_frame(&frame, phases[i].rear);
    }

    state.lighting_rc_valid = true;
    state.link = LINK_STARTING;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    assert_loss_frame(&frame, loss_amber);
    state.link = LINK_STALE;
    lighting_controller_render(&controller, 200u, &state, false, false, &frame);
    assert_loss_frame(&frame, loss_amber);
    state.link = LINK_LOST;
    lighting_controller_render(&controller, 300u, &state, false, false, &frame);
    assert_loss_frame(&frame, black);
}

static void select_mode(LightingController *controller, LightingFrame *frame,
                        VehicleState *state, float aux8, float aux9,
                        uint32_t now_ms)
{
    state->aux8 = aux8;
    state->aux9 = aux9;
    lighting_controller_render(controller, now_ms, state, false, false, frame);
}

static void test_roof_modes_hysteresis_and_animation(void)
{
    static const struct {
        float aux8;
        RoofLightMode mode;
    } bands[] = {
        {-0.875f, ROOF_LIGHT_OFF}, {-0.625f, ROOF_LIGHT_STEADY_WHITE},
        {-0.375f, ROOF_LIGHT_WARM_TRAIL}, {-0.125f, ROOF_LIGHT_BREATHE_AMBER},
        {0.125f, ROOF_LIGHT_COMET}, {0.375f, ROOF_LIGHT_RAINBOW},
        {0.625f, ROOF_LIGHT_DRIVE_SYNC}, {0.875f, ROOF_LIGHT_STATUS},
    };
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    for (size_t i = 0u; i < sizeof bands / sizeof bands[0]; ++i) {
        lighting_controller_init(&controller);
        select_mode(&controller, &frame, &state, bands[i].aux8, 1.0f, 125u);
        assert(frame.roof_mode == bands[i].mode);
        assert(controller.roof_mode == bands[i].mode);
        assert_rear(&frame, (LedRgb){70u, 0u, 0u}, (LedRgb){120u, 0u, 0u},
                    (LedRgb){40u, 0u, 0u}, (LedRgb){40u, 0u, 0u});
        const uint8_t roof_ceiling =
            (bands[i].mode == ROOF_LIGHT_STEADY_WHITE ||
             bands[i].mode == ROOF_LIGHT_WARM_TRAIL ||
             bands[i].mode == ROOF_LIGHT_STATUS ||
             bands[i].mode == ROOF_LIGHT_DRIVE_SYNC) ? 200u : 96u;
        for (size_t index = 0u; index < 16u; ++index) {
            const LedRgb pixel = roof_pixel(&frame, index);
            assert(pixel.r <= roof_ceiling);
            assert(pixel.g <= roof_ceiling);
            assert(pixel.b <= roof_ceiling);
        }
    }

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.625f, 1.0f, 0u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);
    select_mode(&controller, &frame, &state, -0.490f, 1.0f, 0u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);
    select_mode(&controller, &frame, &state, -0.460f, 1.0f, 0u);
    assert(frame.roof_mode == ROOF_LIGHT_WARM_TRAIL);
    select_mode(&controller, &frame, &state, -0.510f, 1.0f, 0u);
    assert(frame.roof_mode == ROOF_LIGHT_WARM_TRAIL);
    select_mode(&controller, &frame, &state, -0.540f, 1.0f, 0u);
    assert(frame.roof_mode == ROOF_LIGHT_STEADY_WHITE);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.625f, -1.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){0u, 0u, 0u});
    select_mode(&controller, &frame, &state, -0.625f, 0.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){100u, 100u, 100u});
    select_mode(&controller, &frame, &state, -0.625f, 1.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){200u, 200u, 200u});

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.375f, 0.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){100u, 50u, 8u});
    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.125f, 1.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){0u, 0u, 0u});
    select_mode(&controller, &frame, &state, -0.125f, 1.0f, 100u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){96u, 32u, 0u});
    select_mode(&controller, &frame, &state, -0.125f, -1.0f, 100u);
    assert(roof_pixel(&frame, 0u).r < 96u);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.125f, 1.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){96u, 48u, 8u});
    select_mode(&controller, &frame, &state, 0.125f, 1.0f, 100u);
    assert_rgb(roof_pixel(&frame, 4u), (LedRgb){96u, 48u, 8u});

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.375f, 1.0f, 0u);
    const LedRgb rainbow_a = roof_pixel(&frame, 0u);
    select_mode(&controller, &frame, &state, 0.375f, 1.0f, 100u);
    const LedRgb rainbow_b = roof_pixel(&frame, 0u);
    assert(memcmp(&rainbow_a, &rainbow_b, sizeof rainbow_a) != 0);

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, 0.625f, 1.0f, 0u);
    assert(roof_pixel(&frame, 0u).r > 0u);
    assert(roof_pixel(&frame, 0u).b == 0u);
    assert(roof_pixel(&frame, 8u).r > 0u);
    assert_current_accounted(&frame);

    lighting_controller_init(&controller);
    state.battery_valid = true;
    state.battery_v = 8.0f;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){0u, 100u, 0u});
    state.battery_v = 6.5f;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 100u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){100u, 33u, 0u});
    state.battery_valid = false;
    select_mode(&controller, &frame, &state, 0.875f, 0.0f, 200u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){0u, 0u, 100u});
}

static void test_brake_reverse_and_rearm(void)
{
    const LedRgb base = {40u, 0u, 0u};
    const LedRgb tail = {70u, 0u, 0u};
    const LedRgb head = {120u, 0u, 0u};
    const LedRgb brake = {200u, 0u, 0u};
    const LedRgb reverse = {160u, 160u, 160u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 100u);
    assert_rear(&frame, head, base, base, tail);
    state.throttle = 0.24f;
    render_rear(&controller, &frame, &state, 110u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 710u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 711u);
    assert_rear(&frame, tail, head, base, base);

    state.throttle = 0.24f;
    render_rear(&controller, &frame, &state, 720u);
    state.throttle = 0.05f;
    render_rear(&controller, &frame, &state, 730u);
    assert_rear(&frame, base, tail, head, base);
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
    assert_rear(&frame, base, base, tail, head);
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
    assert_rear(&frame, head, base, base, tail);

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 1000u);
    state.throttle = -0.50f;
    render_rear(&controller, &frame, &state, 1010u);
    assert_rear(&frame, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 1611u);
    assert_rear(&frame, base, reverse, reverse, base);
}

static void test_turn_and_reverse_turn(void)
{
    const LedRgb base = {40u, 0u, 0u};
    const LedRgb tail = {70u, 0u, 0u};
    const LedRgb head = {120u, 0u, 0u};
    const LedRgb reverse = {160u, 160u, 160u};
    const LedRgb amber = {200u, 70u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 0u);
    assert_rear_side(&frame, 0u, amber, base, base, tail);
    assert_rear_side(&frame, 1u, head, base, base, tail);
    render_rear(&controller, &frame, &state, 160u);
    assert_rear_side(&frame, 0u, amber, amber, amber, base);
    render_rear(&controller, &frame, &state, 332u);
    assert_rear_side(&frame, 0u, amber, amber, amber, amber);
    render_rear(&controller, &frame, &state, 333u);
    assert_rear(&frame, base, tail, head, base);
    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 666u);
    assert_rear_side(&frame, 0u, tail, head, base, base);
    assert_rear_side(&frame, 1u, amber, head, base, base);

    state.throttle = -0.50f;
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 666u);
    assert_rear(&frame, amber, reverse, reverse, base);
    render_rear(&controller, &frame, &state, 999u);
    assert_rear(&frame, base, (LedRgb){199u, 199u, 199u},
                (LedRgb){199u, 199u, 199u}, base);
    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 1332u);
    assert_rear(&frame, base, reverse, reverse, base);
    assert_rear_side(&frame, 1u, amber, reverse, reverse, base);
}

static void test_drive_sync_strips(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    lighting_controller_init(&controller);
    state.aux8 = 0.625f;
    state.aux9 = 1.0f;

    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    assert(frame.roof_mode == ROOF_LIGHT_DRIVE_SYNC);
    for (size_t side = 2u; side < 4u; ++side) {
        for (size_t i = 0u; i < 8u; ++i) {
            const LedRgb p = drive_test_pixel(&frame, side == 3u, i);
            assert(p.r > 0u && p.b == 0u);
        }
    }
    const LedRgb idle0 = drive_test_pixel(&frame, false, 3u);
    lighting_controller_render(&controller, 160u, &state, false, false, &frame);
    const LedRgb idle1 = drive_test_pixel(&frame, false, 3u);
    assert(memcmp(&idle0, &idle1, sizeof idle0) != 0);

    state.throttle = 0.50f;
    lighting_controller_render(&controller, 200u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 0u).g > drive_test_pixel(&frame, false, 7u).g);
    assert(drive_test_pixel(&frame, true, 0u).g > drive_test_pixel(&frame, true, 7u).g);
    assert_current_accounted(&frame);

    state.throttle = -0.50f;
    lighting_controller_render(&controller, 1760u, &state, false, false, &frame);
    lighting_controller_render(&controller, 2400u, &state, false, false, &frame);
    bool saw_reverse_white = false;
    for (size_t i = 0u; i < 8u; ++i) {
        if (drive_test_pixel(&frame, false, i).b > 0u) {
            saw_reverse_white = true;
            assert(drive_test_pixel(&frame, true, i).b > 0u);
        }
    }
    assert(saw_reverse_white);

    state.throttle = 0.0f;
    state.steering = -0.60f;
    lighting_controller_render(&controller, 2500u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 0u).g > 0u);
    assert(drive_test_pixel(&frame, true, 0u).g == 0u);
    assert_current_accounted(&frame);

    state.steering = 0.60f;
    lighting_controller_render(&controller, 2600u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 0u).g == 0u);
    assert(drive_test_pixel(&frame, true, 0u).g > 0u);

    state.steering = 0.0f;
    state.throttle = 0.50f;
    lighting_controller_render(&controller, 2700u, &state, false, false, &frame);
    state.throttle = 0.0f;
    lighting_controller_render(&controller, 2710u, &state, false, false, &frame);
    assert(controller.brake_active);
    for (size_t i = 0u; i < 8u; ++i) {
        assert(drive_test_pixel(&frame, false, i).r > 0u);
        assert(drive_test_pixel(&frame, false, i).g == 0u);
    }
    assert_current_accounted(&frame);

    lighting_controller_render(&controller, 3110u, &state, false, false, &frame);
    assert(controller.brake_active);
    for (size_t i = 0u; i < 8u; ++i) {
        assert_rgb(drive_test_pixel(&frame, false, i), (LedRgb){200u, 0u, 0u});
        assert_rgb(drive_test_pixel(&frame, true, i), (LedRgb){200u, 0u, 0u});
    }

    lighting_controller_init(&controller);
    state = lighting_state();
    state.aux8 = 0.625f;
    state.aux9 = 1.0f;
    state.throttle = -0.5f;
    state.steering = -0.6f;
    lighting_controller_render(&controller, 0u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 0u).g > 0u);
    assert(drive_test_pixel(&frame, false, 7u).b > 0u);
    lighting_controller_render(&controller, 300u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 7u).g > 0u);
    assert(drive_test_pixel(&frame, false, 5u).b > 0u);
    lighting_controller_render(&controller, 400u, &state, false, false, &frame);
    assert(drive_test_pixel(&frame, false, 0u).g == 0u);
    assert(drive_test_pixel(&frame, false, 4u).b > 0u);
    assert_current_accounted(&frame);
}

static void test_turn_overlays_brake_on_selected_side_only(void)
{
    const LedRgb amber = {200u, 70u, 0u};
    const LedRgb brake = {200u, 0u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 0u);
    state.throttle = 0.10f;
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 10u);
    assert_rear_side(&frame, 0u, amber, brake, brake, brake);
    assert_rear_side(&frame, 1u, brake, brake, brake, brake);
    render_rear(&controller, &frame, &state, 130u);
    assert_rear_side(&frame, 0u, amber, amber,
                     (LedRgb){150u, 0u, 0u}, (LedRgb){150u, 0u, 0u});
    assert_rear_side(&frame, 1u, (LedRgb){150u, 0u, 0u},
                     (LedRgb){150u, 0u, 0u}, (LedRgb){150u, 0u, 0u},
                     (LedRgb){150u, 0u, 0u});
    assert_current_accounted(&frame);
}

static void test_static_roof_reaches_brighter_level_with_current_accounting(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    select_mode(&controller, &frame, &state, -0.625f, 1.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){200u, 200u, 200u});
    assert_rgb(roof_pixel(&frame, 15u), (LedRgb){200u, 200u, 200u});
    assert_current_accounted(&frame);

    select_mode(&controller, &frame, &state, -0.375f, 0.0f, 0u);
    assert_rgb(roof_pixel(&frame, 0u), (LedRgb){100u, 50u, 8u});
    assert_current_accounted(&frame);
}

static void test_rear_running_chase_uses_same_id_order_on_both_sides(void)
{
    const LedRgb base = {40u, 0u, 0u};
    const LedRgb tail = {70u, 0u, 0u};
    const LedRgb head = {120u, 0u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    render_rear(&controller, &frame, &state, 0u);
    for (size_t side = 0u; side < 2u; ++side) {
        assert_rear_side(&frame, side, head, base, base, tail);
    }
    render_rear(&controller, &frame, &state, 120u);
    for (size_t side = 0u; side < 2u; ++side) {
        assert_rear_side(&frame, side, tail, head, base, base);
    }
    render_rear(&controller, &frame, &state, 480u);
    for (size_t side = 0u; side < 2u; ++side) {
        assert_rear_side(&frame, side, head, base, base, tail);
    }
    assert_current_accounted(&frame);
}

static void test_brake_stays_bright_after_two_pulses(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    render_rear(&controller, &frame, &state, 100u);
    state.throttle = 0.10f;
    render_rear(&controller, &frame, &state, 110u);
    assert_rear_groups(&frame, (LedRgb){200u, 0u, 0u});
    render_rear(&controller, &frame, &state, 230u);
    assert_rear_groups(&frame, (LedRgb){150u, 0u, 0u});
    render_rear(&controller, &frame, &state, 330u);
    assert_rear_groups(&frame, (LedRgb){200u, 0u, 0u});
    render_rear(&controller, &frame, &state, 390u);
    assert_rear_groups(&frame, (LedRgb){150u, 0u, 0u});
    render_rear(&controller, &frame, &state, 430u);
    assert_rear_groups(&frame, (LedRgb){200u, 0u, 0u});
    render_rear(&controller, &frame, &state, 710u);
    assert_rear_groups(&frame, (LedRgb){200u, 0u, 0u});
    assert_current_accounted(&frame);
}

static void test_reverse_keeps_white_identity_during_ping(void)
{
    const LedRgb red = {40u, 0u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.throttle = -0.50f;
    render_rear(&controller, &frame, &state, 0u);
    for (size_t side = 0u; side < 2u; ++side) {
        assert_rear_side(&frame, side, red, (LedRgb){160u, 160u, 160u},
                         (LedRgb){160u, 160u, 160u}, red);
    }
    render_rear(&controller, &frame, &state, 200u);
    for (size_t side = 0u; side < 2u; ++side) {
        assert_rear_side(&frame, side, red, (LedRgb){200u, 200u, 200u},
                         (LedRgb){200u, 200u, 200u}, red);
    }
    assert_current_accounted(&frame);
}

static void test_turn_sweep_is_side_specific_and_preserves_reverse_white(void)
{
    const LedRgb red = {40u, 0u, 0u};
    const LedRgb amber = {200u, 70u, 0u};
    const LedRgb white = {160u, 160u, 160u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    state.steering = -0.50f;
    state.throttle = -0.50f;
    render_rear(&controller, &frame, &state, 0u);
    assert_rear_side(&frame, 0u, amber, white, white, red);
    assert_rear_side(&frame, 1u, red, white, white, red);
    render_rear(&controller, &frame, &state, 252u);
    assert_rear_side(&frame, 0u, amber, white, white, amber);
    assert_rear_side(&frame, 1u, red, white, white, red);
    render_rear(&controller, &frame, &state, 333u);
    assert_rear_side(&frame, 0u, red, white, white, red);

    state.throttle = 0.0f;
    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 666u);
    assert_rear_side(&frame, 1u, amber, (LedRgb){120u, 0u, 0u}, red, red);
    assert_current_accounted(&frame);
}

static void test_turn_sweep_starts_immediately_after_steering_change(void)
{
    const LedRgb amber = {200u, 70u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();

    lighting_controller_init(&controller);
    render_rear(&controller, &frame, &state, 333u);
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 334u);
    assert_rgb(frame.ws2812.groups[0][0], amber);
    assert(frame.ws2812.groups[0][1].g == 0u);
    assert(frame.ws2812.groups[1][0].g == 0u);

    render_rear(&controller, &frame, &state, 414u);
    assert_rgb(frame.ws2812.groups[0][0], amber);
    assert_rgb(frame.ws2812.groups[0][1], amber);
    render_rear(&controller, &frame, &state, 667u);
    assert(frame.ws2812.groups[0][0].g == 0u);

    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 700u);
    assert_rgb(frame.ws2812.groups[1][0], amber);
    assert(frame.ws2812.groups[0][0].g == 0u);

    state.steering = 0.0f;
    render_rear(&controller, &frame, &state, 701u);
    state.steering = 0.50f;
    render_rear(&controller, &frame, &state, 1000u);
    assert_rgb(frame.ws2812.groups[1][0], amber);
    assert(frame.ws2812.groups[1][1].g == 0u);

    state.link = LINK_STALE;
    render_rear(&controller, &frame, &state, 1001u);
    state.link = LINK_OK;
    render_rear(&controller, &frame, &state, 1334u);
    assert_rgb(frame.ws2812.groups[1][0], amber);
    assert(frame.ws2812.groups[1][1].g == 0u);
    assert_current_accounted(&frame);
}

static void test_peak_rear_and_roof_effects_keep_estimated_margin(void)
{
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    state.aux8 = -0.625f;
    state.aux9 = 1.0f;
    state.throttle = -0.50f;

    lighting_controller_init(&controller);
    render_rear(&controller, &frame, &state, 39959u);
    state.steering = -0.50f;
    render_rear(&controller, &frame, &state, 39960u);
    render_rear(&controller, &frame, &state, 40200u);

    assert(frame.estimated_ma <= 850u);
    assert(frame.ws2812.groups[0][0].g > 0u);
    assert(frame.ws2812.groups[0][1].g > 100u);
    assert(frame.ws2812.groups[1][1].g > 100u);
    assert_current_accounted(&frame);
}

static void test_warning_pulses_and_priority(void)
{
    const LedRgb black = {0u, 0u, 0u};
    const LedRgb board_red = {64u, 0u, 0u};
    const LedRgb battery_orange = {64u, 8u, 0u};
    LightingController controller;
    LightingFrame frame;
    VehicleState state = lighting_state();
    state.aux8 = -0.625f;
    state.aux9 = 1.0f;

    lighting_controller_init(&controller);
    const uint32_t board_on[] = {0u, 200u, 400u};
    for (size_t i = 0u; i < sizeof board_on / sizeof board_on[0]; ++i) {
        lighting_controller_render(&controller, board_on[i], &state, false,
                                   true, &frame);
        assert_rear_groups(&frame, board_red);
        assert_roof_off(&frame);
    }
    lighting_controller_render(&controller, 100u, &state, false, true, &frame);
    assert_rear_groups(&frame, black);
    lighting_controller_render(&controller, 500u, &state, false, true, &frame);
    assert_rear_groups(&frame, black);

    lighting_controller_init(&controller);
    const uint32_t battery_on[] = {0u, 400u, 800u};
    for (size_t i = 0u; i < sizeof battery_on / sizeof battery_on[0]; ++i) {
        lighting_controller_render(&controller, battery_on[i], &state, true,
                                   false, &frame);
        assert_rear_groups(&frame, battery_orange);
        assert_roof_off(&frame);
    }
    lighting_controller_render(&controller, 200u, &state, true, false, &frame);
    assert_rear_groups(&frame, black);
    lighting_controller_render(&controller, 1000u, &state, true, false, &frame);
    assert_rear_groups(&frame, black);

    lighting_controller_init(&controller);
    state.throttle = 0.40f;
    lighting_controller_render(&controller, 390u, &state, false, false, &frame);
    state.throttle = -0.50f;
    state.steering = -0.50f;
    lighting_controller_render(&controller, 400u, &state, true, true, &frame);
    assert_rear_groups(&frame, board_red);
    assert_roof_off(&frame);
    lighting_controller_render(&controller, 400u, &state, true, false, &frame);
    assert_rear_groups(&frame, battery_orange);
    assert_roof_off(&frame);
    lighting_controller_render(&controller, 400u, &state, false, false, &frame);
    assert(frame.ws2812.groups[0][0].g > 0u);
    assert(frame.ws2812.groups[1][0].r >= 150u);
    assert(frame.ws2812.groups[1][0].g == 0u);
    assert(roof_pixel(&frame, 0u).r > 150u);
    assert_current_accounted(&frame);
    lighting_controller_render(&controller, 700u, &state, true, true, &frame);
    assert_rear_groups(&frame, black);
    assert_roof_off(&frame);
}

void test_lighting_controller(void)
{
    test_rear_running_chase_uses_same_id_order_on_both_sides();
    test_brake_stays_bright_after_two_pulses();
    test_reverse_keeps_white_identity_during_ping();
    test_turn_sweep_is_side_specific_and_preserves_reverse_white();
    test_turn_sweep_starts_immediately_after_steering_change();
    test_peak_rear_and_roof_effects_keep_estimated_margin();
    test_turn_overlays_brake_on_selected_side_only();
    test_static_roof_reaches_brighter_level_with_current_accounting();
    test_four_group_contract();
    test_startup_frame_is_black_before_valid_lighting_rc();
    test_roof_animation_spans_two_independent_groups();
    test_duties_clamp_and_loss_warning_preserves_roof_black();
    test_warning_overrides_both_rear_groups();
    test_fail_safe_duties_and_bounds();
    test_startup_black_and_post_valid_lighting_loss();
    test_roof_modes_hysteresis_and_animation();
    test_drive_sync_strips();
    test_brake_reverse_and_rearm();
    test_turn_and_reverse_turn();
    test_warning_pulses_and_priority();
}
