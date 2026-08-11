#include <assert.h>
#include <string.h>

#include "led/led_controller.h"

static void test_current_estimation(void)
{
    LedRgb pixels[LED_MAX_PIXELS + 1u] = {0};

    pixels[0] = (LedRgb){255u, 255u, 255u};
    pixels[LED_MAX_PIXELS] = (LedRgb){255u, 255u, 255u};

    assert(led_estimated_ma(pixels, 1u) == 60u);
    assert(led_estimated_ma(pixels, LED_MAX_PIXELS + 1u) == 60u);
    assert(led_estimated_ma(pixels, 0u) == 0u);
    assert(led_estimated_ma(NULL, LED_MAX_PIXELS) == 0u);
}

static void test_current_limiting(void)
{
    LedRgb pixels[LED_MAX_PIXELS + 1u];
    const LedRgb guard = {0xA5u, 0x5Au, 0xC3u};

    for (size_t i = 0u; i < LED_MAX_PIXELS; ++i) {
        pixels[i] = (LedRgb){255u, 255u, 255u};
    }
    pixels[LED_MAX_PIXELS] = guard;

    assert(led_estimated_ma(pixels, LED_MAX_PIXELS) == 1800u);
    led_limit_current(pixels, LED_MAX_PIXELS + 1u,
                      LED_CURRENT_BUDGET_MA);
    assert(led_estimated_ma(pixels, LED_MAX_PIXELS) <=
           LED_CURRENT_BUDGET_MA);
    assert(memcmp(&pixels[LED_MAX_PIXELS], &guard, sizeof guard) == 0);

    led_limit_current(NULL, LED_MAX_PIXELS, LED_CURRENT_BUDGET_MA);
    led_limit_current(pixels, 0u, LED_CURRENT_BUDGET_MA);
}

void test_led_controller(void)
{
    test_current_estimation();
    test_current_limiting();
}
