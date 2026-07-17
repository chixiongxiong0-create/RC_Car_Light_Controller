#include "led/led_controller.h"

static LedRgb page_color(UiPage page)
{
    if (page == UI_PAGE_FACE) {
        return (LedRgb){0u, 12u, 24u};
    }
    if (page == UI_PAGE_SHOWCASE) {
        return (LedRgb){0u, 0u, 8u};
    }
    return (LedRgb){24u, 18u, 0u};
}

static bool pulse_on(uint32_t phase_ms, uint32_t on_ms,
                     uint32_t spacing_ms, unsigned pulses)
{
    for (unsigned i = 0u; i < pulses; ++i) {
        const uint32_t start = i * spacing_ms;
        if (phase_ms >= start && phase_ms < start + on_ms) {
            return true;
        }
    }
    return false;
}

void led_controller_render(uint32_t now_ms, const VehicleState *state,
                           UiPage page, bool low_battery, bool board_fault,
                           LedRgb out[LED_MAX_PIXELS], size_t count)
{
    if (out == NULL || state == NULL) {
        return;
    }
    if (count > LED_MAX_PIXELS) {
        count = LED_MAX_PIXELS;
    }

    LedRgb base = page_color(page);
    bool left_only = false;
    bool right_only = false;
    if (board_fault) {
        base = (LedRgb){64u, 0u, 64u};
    } else if (state->link == LINK_LOST) {
        base = pulse_on(now_ms % 2000u, 100u, 200u, 2u)
                   ? (LedRgb){64u, 0u, 0u} : (LedRgb){0u, 0u, 0u};
    } else if (low_battery) {
        base = pulse_on(now_ms % 5000u, 200u, 400u, 3u)
                   ? (LedRgb){64u, 8u, 0u} : (LedRgb){0u, 0u, 0u};
    } else if (state->steering < -0.25f) {
        base = (LedRgb){64u, 24u, 0u};
        left_only = true;
    } else if (state->steering > 0.25f) {
        base = (LedRgb){64u, 24u, 0u};
        right_only = true;
    } else if (state->throttle < -0.20f) {
        base = (LedRgb){48u, 0u, 0u};
    }

    const LedRgb inactive = page_color(page);
    const size_t half = count / 2u;
    for (size_t i = 0u; i < count; ++i) {
        const bool active = (!left_only && !right_only) ||
                            (left_only && i < half) ||
                            (right_only && i >= half);
        out[i] = active ? base : inactive;
    }
}

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
