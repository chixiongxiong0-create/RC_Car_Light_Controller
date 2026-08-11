#include "led/led_controller.h"

uint32_t led_estimated_ma(const LedRgb *pixels, size_t count)
{
    if (pixels == NULL) {
        return 0u;
    }
    if (count > LED_MAX_PIXELS) {
        count = LED_MAX_PIXELS;
    }
    uint32_t channel_sum = 0u;
    for (size_t i = 0u; i < count; ++i) {
        channel_sum += pixels[i].r + pixels[i].g + pixels[i].b;
    }
    return (channel_sum * 20u) / 255u;
}

void led_limit_current(LedRgb *pixels, size_t count, uint32_t budget_ma)
{
    if (pixels == NULL) {
        return;
    }
    if (count > LED_MAX_PIXELS) {
        count = LED_MAX_PIXELS;
    }
    const uint32_t estimated = led_estimated_ma(pixels, count);
    if (estimated <= budget_ma || estimated == 0u) {
        return;
    }
    for (size_t i = 0u; i < count; ++i) {
        pixels[i].r = (uint8_t)(((uint32_t)pixels[i].r * budget_ma) / estimated);
        pixels[i].g = (uint8_t)(((uint32_t)pixels[i].g * budget_ma) / estimated);
        pixels[i].b = (uint8_t)(((uint32_t)pixels[i].b * budget_ma) / estimated);
    }
}
