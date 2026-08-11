#include <assert.h>
#include <string.h>

#include "lighting/lighting_service.h"

typedef struct {
    size_t apply_calls;
    uint16_t front_duties[4];
    uint16_t roof_duties[4];
    size_t submit_calls;
    uint32_t submitted_at[4];
    size_t submitted_counts[4];
    LedRgb submitted_pixels[4][LED_MAX_PIXELS];
} LightingCapture;

static void capture_apply(uint16_t front_duty, uint16_t roof_spot_duty,
                          void *ctx)
{
    LightingCapture *capture = ctx;
    assert(capture->apply_calls < 4u);
    capture->front_duties[capture->apply_calls] = front_duty;
    capture->roof_duties[capture->apply_calls] = roof_spot_duty;
    ++capture->apply_calls;
}

static bool capture_submit(uint32_t now_ms, const LedRgb *pixels,
                           size_t count, void *ctx)
{
    LightingCapture *capture = ctx;
    assert(capture->submit_calls < 4u);
    assert(count <= LED_MAX_PIXELS);
    capture->submitted_at[capture->submit_calls] = now_ms;
    capture->submitted_counts[capture->submit_calls] = count;
    memcpy(capture->submitted_pixels[capture->submit_calls], pixels,
           count * sizeof *pixels);
    ++capture->submit_calls;
    return false;
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

static void assert_rgb(LedRgb actual, LedRgb expected)
{
    assert(actual.r == expected.r);
    assert(actual.g == expected.g);
    assert(actual.b == expected.b);
}

static void test_service_delivers_one_combined_post_limit_frame(void)
{
    LightingService service;
    LightingCapture capture = {0};
    VehicleState state = valid_real_state();

    lighting_service_init(&service);
    const uint32_t estimated_ma = lighting_service_tick(
        &service, 123u, &state, false, false, 6u,
        capture_apply, capture_submit, &capture);

    assert(capture.apply_calls == 1u);
    assert(capture.front_duties[0] == 1000u);
    assert(capture.roof_duties[0] == 500u);
    assert(capture.submit_calls == 1u);
    assert(capture.submitted_at[0] == 123u);
    assert(capture.submitted_counts[0] == 6u);
    for (size_t i = 0u; i < 4u; ++i) {
        assert_rgb(capture.submitted_pixels[0][i],
                   (LedRgb){12u, 0u, 0u});
    }
    assert_rgb(capture.submitted_pixels[0][4],
               (LedRgb){96u, 96u, 96u});
    assert_rgb(capture.submitted_pixels[0][5],
               (LedRgb){96u, 96u, 96u});
    assert(memcmp(capture.submitted_pixels[0], service.frame.pixels,
                  6u * sizeof(LedRgb)) == 0);
    assert(estimated_ma == 48u);
    assert(estimated_ma == service.frame.estimated_ma);
}

static void test_service_turns_high_power_off_and_submits_loss_warning(void)
{
    LightingService service;
    LightingCapture capture = {0};
    VehicleState state = valid_real_state();
    const LinkState invalid_links[] = {
        LINK_STARTING,
        LINK_STALE,
        LINK_LOST,
    };

    lighting_service_init(&service);
    (void)lighting_service_tick(&service, 99u, &state, false, false, 6u,
                                capture_apply, capture_submit, &capture);
    for (size_t i = 0u; i < 3u; ++i) {
        state.link = invalid_links[i];
        assert(lighting_service_tick(&service, (uint32_t)i, &state,
                                     false, false, 6u,
                                     capture_apply, capture_submit,
                                     &capture) == 12u);
    }

    assert(capture.front_duties[0] == 1000u);
    assert(capture.roof_duties[0] == 500u);
    assert(capture.apply_calls == 4u);
    assert(capture.submit_calls == 4u);
    for (size_t call = 1u; call < 4u; ++call) {
        assert(capture.front_duties[call] == 0u);
        assert(capture.roof_duties[call] == 0u);
        assert(capture.submitted_counts[call] == 6u);
        for (size_t pixel = 0u; pixel < 4u; ++pixel) {
            assert_rgb(capture.submitted_pixels[call][pixel],
                       (LedRgb){32u, 8u, 0u});
        }
        for (size_t pixel = 4u; pixel < 6u; ++pixel) {
            assert_rgb(capture.submitted_pixels[call][pixel],
                       (LedRgb){0u, 0u, 0u});
        }
    }
}

void test_lighting_service(void)
{
    test_service_delivers_one_combined_post_limit_frame();
    test_service_turns_high_power_off_and_submits_loss_warning();
}
