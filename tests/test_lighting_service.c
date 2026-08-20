#include <assert.h>
#include <string.h>

#include "lighting/lighting_service.h"

typedef struct {
    size_t apply_calls;
    uint16_t front_duty;
    uint16_t roof_duty;
    size_t calls;
    uint32_t now_ms;
    Ws2812Frame frame;
} Capture;

static void capture_apply(uint16_t front_duty, uint16_t roof_spot_duty,
                          void *ctx)
{
    Capture *capture = ctx;
    capture->front_duty = front_duty;
    capture->roof_duty = roof_spot_duty;
    capture->apply_calls++;
}

static bool capture_groups(uint32_t now_ms, const Ws2812Frame *frame, void *ctx)
{
    Capture *capture = ctx;
    capture->now_ms = now_ms;
    capture->frame = *frame;
    capture->calls++;
    return true;
}

static VehicleState valid_real_state(void)
{
    return (VehicleState){
        .aux6 = 1.0f,
        .aux7 = 0.0f,
        .aux8 = -0.70f,
        .aux9 = 1.0f,
        .battery_v = 8.0f,
        .battery_valid = true,
        .lighting_rc_valid = true,
        .link = LINK_OK,
    };
}

static void assert_groups_black(const Ws2812Frame *frame)
{
    for (size_t group = 0u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            assert(frame->groups[group][pixel].r == 0u);
            assert(frame->groups[group][pixel].g == 0u);
            assert(frame->groups[group][pixel].b == 0u);
        }
    }
}

static void test_service_submits_one_complete_frame_per_tick(void)
{
    LightingService service;
    Capture capture = {0};
    VehicleState state = valid_real_state();

    lighting_service_init(&service);
    const uint32_t estimated_ma = lighting_service_tick(
        &service, 123u, &state, false, false,
        capture_apply, capture_groups, &capture);

    assert(capture.apply_calls == 1u);
    assert(capture.front_duty == 1000u);
    assert(capture.roof_duty == 500u);
    assert(capture.calls == 1u);
    assert(capture.now_ms == 123u);
    assert(memcmp(&capture.frame, &service.frame.ws2812,
                  sizeof capture.frame) == 0);
    assert(estimated_ma == service.frame.estimated_ma);
}

static void test_service_submits_black_groups_before_valid_lighting_rc(void)
{
    LightingService service;
    Capture capture = {0};
    VehicleState state = valid_real_state();

    lighting_service_init(&service);
    state.lighting_rc_valid = false;
    assert(lighting_service_tick(&service, 17u, &state, false, false,
                                 capture_apply, capture_groups,
                                 &capture) == 0u);

    assert(capture.apply_calls == 1u);
    assert(capture.front_duty == 0u);
    assert(capture.roof_duty == 0u);
    assert(capture.calls == 1u);
    assert(capture.now_ms == 17u);
    assert_groups_black(&capture.frame);
}

static void test_service_submits_one_loss_frame_per_tick(void)
{
    LightingService service;
    Capture capture = {0};
    VehicleState state = valid_real_state();

    lighting_service_init(&service);
    (void)lighting_service_tick(&service, 0u, &state, false, false,
                                capture_apply, capture_groups, &capture);
    state.link = LINK_LOST;
    (void)lighting_service_tick(&service, 0u, &state, false, false,
                                capture_apply, capture_groups, &capture);

    assert(capture.apply_calls == 2u);
    assert(capture.calls == 2u);
    for (size_t pixel = 0u; pixel < 4u; ++pixel) {
        assert(capture.frame.groups[0][pixel].r == 32u);
        assert(capture.frame.groups[0][pixel].g == 8u);
        assert(capture.frame.groups[1][pixel].r == 32u);
        assert(capture.frame.groups[1][pixel].g == 8u);
    }
    for (size_t pixel = 0u; pixel < 8u; ++pixel) {
        assert(capture.frame.groups[2][pixel].r == 0u);
        assert(capture.frame.groups[3][pixel].r == 0u);
    }
}

void test_lighting_service(void)
{
    test_service_submits_one_complete_frame_per_tick();
    test_service_submits_black_groups_before_valid_lighting_rc();
    test_service_submits_one_loss_frame_per_tick();
}
