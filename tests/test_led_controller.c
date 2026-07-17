#include <assert.h>
#include <string.h>

#include "led/led_controller.h"

static VehicleState nominal_state(void)
{
    VehicleState state = {0};
    state.link = LINK_OK;
    return state;
}

static void assert_all(LedRgb *pixels, size_t count, LedRgb expected)
{
    for (size_t i = 0; i < count; ++i) {
        assert(pixels[i].r == expected.r);
        assert(pixels[i].g == expected.g);
        assert(pixels[i].b == expected.b);
    }
}

void test_led_controller(void)
{
    LedRgb pixels[LED_MAX_PIXELS];
    VehicleState state = nominal_state();

    led_controller_render(0u, &state, UI_PAGE_DASHBOARD, false, false,
                          pixels, LED_MAX_PIXELS);
    assert_all(pixels, LED_MAX_PIXELS, (LedRgb){24u, 18u, 0u});

    state.throttle = -0.5f;
    led_controller_render(0u, &state, UI_PAGE_FACE, false, false,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){48u, 0u, 0u});

    state.throttle = 0.0f;
    state.steering = -0.5f;
    led_controller_render(0u, &state, UI_PAGE_SHOWCASE, false, false,
                          pixels, 10u);
    assert_all(pixels, 5u, (LedRgb){64u, 24u, 0u});
    assert_all(pixels + 5u, 5u, (LedRgb){0u, 0u, 8u});

    /* Low battery overrides steering and flashes three times every 5 seconds. */
    led_controller_render(0u, &state, UI_PAGE_SHOWCASE, true, false,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){64u, 8u, 0u});
    led_controller_render(700u, &state, UI_PAGE_SHOWCASE, true, false,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){0u, 0u, 0u});

    /* Link loss overrides low battery and uses a double flash. */
    state.link = LINK_LOST;
    led_controller_render(0u, &state, UI_PAGE_SHOWCASE, true, false,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){64u, 0u, 0u});
    led_controller_render(300u, &state, UI_PAGE_SHOWCASE, true, false,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){0u, 0u, 0u});

    /* Board fault is the top-priority solid indication. */
    led_controller_render(300u, &state, UI_PAGE_SHOWCASE, true, true,
                          pixels, 10u);
    assert_all(pixels, 10u, (LedRgb){64u, 0u, 64u});

    LedRgb full[LED_MAX_PIXELS];
    for (size_t i = 0; i < LED_MAX_PIXELS; ++i) {
        full[i] = (LedRgb){255u, 255u, 255u};
    }
    assert(led_estimated_ma(full, LED_MAX_PIXELS) == 1800u);
    led_limit_current(full, LED_MAX_PIXELS, LED_CURRENT_BUDGET_MA);
    assert(led_estimated_ma(full, LED_MAX_PIXELS) <= LED_CURRENT_BUDGET_MA);

    LedRgb guards[LED_MAX_PIXELS + 2u];
    memset(guards, 0xA5, sizeof guards);
    led_controller_render(0u, &state, UI_PAGE_DASHBOARD, false, false,
                          guards + 1u, LED_MAX_PIXELS + 10u);
    assert(memcmp(&guards[0], &(LedRgb){0xA5u, 0xA5u, 0xA5u}, sizeof(LedRgb)) == 0);
    assert(memcmp(&guards[LED_MAX_PIXELS + 1u],
                  &(LedRgb){0xA5u, 0xA5u, 0xA5u}, sizeof(LedRgb)) == 0);

    /* Zero count and NULL input are valid no-op edge cases. */
    led_controller_render(0u, NULL, UI_PAGE_DASHBOARD, false, false,
                          pixels, 0u);
    assert(led_estimated_ma(NULL, 0u) == 0u);
}
