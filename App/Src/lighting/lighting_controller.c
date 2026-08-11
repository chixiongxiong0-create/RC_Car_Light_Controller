#include "lighting/lighting_controller.h"

#include <string.h>

static float clamp_normalized(float value)
{
    if (value < -1.0f) {
        return -1.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static uint16_t normalized_to_duty(float value)
{
    const float unit = (clamp_normalized(value) + 1.0f) * 0.5f;
    return (uint16_t)(unit * 1000.0f + 0.5f);
}

static uint8_t normalized_to_level(float value)
{
    const float unit = (clamp_normalized(value) + 1.0f) * 0.5f;
    return (uint8_t)(unit * 96.0f + 0.5f);
}

static RoofLightMode mode_for_aux(float value)
{
    const float scaled = (clamp_normalized(value) + 1.0f) * 4.0f;
    unsigned mode = (unsigned)scaled;
    if (mode >= (unsigned)ROOF_LIGHT_MODE_COUNT) {
        mode = (unsigned)ROOF_LIGHT_STATUS;
    }
    return (RoofLightMode)mode;
}

static void update_roof_mode(LightingController *controller, float aux8)
{
    const float value = clamp_normalized(aux8);
    const RoofLightMode candidate = mode_for_aux(value);
    const float hysteresis = 0.03f;

    if (candidate > controller->roof_mode) {
        const float boundary = -1.0f +
            0.25f * (float)((unsigned)controller->roof_mode + 1u);
        if (value >= boundary + hysteresis) {
            controller->roof_mode = candidate;
        }
    } else if (candidate < controller->roof_mode) {
        const float boundary = -1.0f +
            0.25f * (float)(unsigned)controller->roof_mode;
        if (value <= boundary - hysteresis) {
            controller->roof_mode = candidate;
        }
    }
}

static LedRgb scale_color(LedRgb color, uint8_t level)
{
    color.r = (uint8_t)(((uint16_t)color.r * level) / 96u);
    color.g = (uint8_t)(((uint16_t)color.g * level) / 96u);
    color.b = (uint8_t)(((uint16_t)color.b * level) / 96u);
    return color;
}

static uint32_t animation_period(float aux9)
{
    const uint16_t speed = normalized_to_duty(aux9);
    return 1600u - ((uint32_t)speed * 1200u) / 1000u;
}

static LedRgb rainbow_color(uint8_t position)
{
    if (position < 85u) {
        const uint8_t rising = (uint8_t)(((uint16_t)position * 96u) / 85u);
        return (LedRgb){(uint8_t)(96u - rising), rising, 0u};
    }
    if (position < 170u) {
        const uint8_t offset = (uint8_t)(position - 85u);
        const uint8_t rising = (uint8_t)(((uint16_t)offset * 96u) / 85u);
        return (LedRgb){0u, (uint8_t)(96u - rising), rising};
    }
    const uint8_t offset = (uint8_t)(position - 170u);
    const uint8_t rising = (uint8_t)(((uint16_t)offset * 96u) / 85u);
    return (LedRgb){rising, 0u, (uint8_t)(96u - rising)};
}

static void render_roof(const LightingController *controller,
                        uint32_t now_ms,
                        const VehicleState *state,
                        LightingFrame *frame,
                        size_t pixel_count)
{
    const size_t roof_count = pixel_count - 4u;
    const uint8_t level = normalized_to_level(state->aux9);
    const uint32_t period = animation_period(state->aux9);

    if (controller->roof_mode == ROOF_LIGHT_OFF || roof_count == 0u) {
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_STEADY_WHITE) {
        const LedRgb color = (LedRgb){level, level, level};
        for (size_t i = 4u; i < pixel_count; ++i) {
            frame->pixels[i] = color;
        }
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_WARM_TRAIL) {
        const LedRgb color = scale_color((LedRgb){96u, 48u, 8u}, level);
        for (size_t i = 4u; i < pixel_count; ++i) {
            frame->pixels[i] = color;
        }
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_BREATHE_AMBER) {
        const uint32_t breathe_period = period / 2u;
        const uint32_t phase = now_ms % breathe_period;
        const uint32_t half = breathe_period / 2u;
        const uint8_t breathe = phase <= half
            ? (uint8_t)((phase * 96u) / half)
            : (uint8_t)(((breathe_period - phase) * 96u) / half);
        const LedRgb color = scale_color((LedRgb){96u, 32u, 0u}, breathe);
        for (size_t i = 4u; i < pixel_count; ++i) {
            frame->pixels[i] = color;
        }
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_COMET) {
        const size_t head = ((now_ms % period) * roof_count) / period;
        const size_t tail = (head + roof_count - 1u) % roof_count;
        frame->pixels[4u + head] = (LedRgb){96u, 48u, 8u};
        frame->pixels[4u + tail] = (LedRgb){24u, 8u, 1u};
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_RAINBOW) {
        const uint32_t phase = ((now_ms % period) * 256u) / period;
        for (size_t i = 0u; i < roof_count; ++i) {
            const uint32_t offset = (i * 256u) / roof_count;
            frame->pixels[4u + i] = rainbow_color((uint8_t)(phase + offset));
        }
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_POLICE) {
        const bool swapped = ((now_ms / (period / 2u)) & 1u) != 0u;
        for (size_t i = 0u; i < roof_count; ++i) {
            const bool red = ((i & 1u) == 0u) != swapped;
            frame->pixels[4u + i] = red
                ? (LedRgb){96u, 0u, 0u} : (LedRgb){0u, 0u, 96u};
        }
        return;
    }

    LedRgb status;
    if (!state->battery_valid) {
        status = (LedRgb){0u, 0u, level};
    } else if (state->battery_v < 7.0f) {
        status = (LedRgb){level, (uint8_t)(level / 3u), 0u};
    } else {
        status = (LedRgb){0u, level, 0u};
    }
    for (size_t i = 4u; i < pixel_count; ++i) {
        frame->pixels[i] = status;
    }
}

static void update_brake(LightingController *controller,
                         uint32_t now_ms,
                         float throttle)
{
    if (controller->brake_active &&
        (uint32_t)(now_ms - controller->brake_trigger_ms) > 600u) {
        controller->brake_active = false;
    }

    bool triggered = false;
    if (controller->forward_armed && controller->has_previous_throttle &&
        ((controller->previous_throttle - throttle) >= 0.15f ||
         throttle <= 0.10f)) {
        controller->brake_active = true;
        controller->brake_trigger_ms = now_ms;
        controller->forward_armed = false;
        triggered = true;
    }
    if (!triggered && throttle > 0.25f) {
        controller->forward_armed = true;
    }
    controller->previous_throttle = throttle;
    controller->has_previous_throttle = true;
}

static void render_rear(const LightingController *controller,
                        uint32_t now_ms,
                        const VehicleState *state,
                        LightingFrame *frame)
{
    const LedRgb base = {12u, 0u, 0u};
    const LedRgb brake = {96u, 0u, 0u};
    const LedRgb reverse_color = {96u, 96u, 96u};
    const LedRgb amber = {96u, 32u, 0u};

    for (size_t i = 0u; i < 4u; ++i) {
        frame->pixels[i] = base;
    }
    if (controller->brake_active) {
        for (size_t i = 0u; i < 4u; ++i) {
            frame->pixels[i] = brake;
        }
        return;
    }

    const bool reverse = state->throttle < -0.20f;
    if (reverse) {
        frame->pixels[1] = reverse_color;
        frame->pixels[2] = reverse_color;
    }

    const bool blink_on = (now_ms % 666u) < 333u;
    if (!blink_on) {
        return;
    }
    if (state->steering < -0.30f) {
        frame->pixels[0] = amber;
        if (!reverse) {
            frame->pixels[1] = amber;
        }
    } else if (state->steering > 0.30f) {
        if (!reverse) {
            frame->pixels[2] = amber;
        }
        frame->pixels[3] = amber;
    }
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

static bool render_warning(uint32_t now_ms,
                           bool low_battery,
                           bool board_fault,
                           LightingFrame *frame)
{
    LedRgb color;
    if (board_fault) {
        color = pulse_on(now_ms % 2000u, 100u, 200u, 3u)
            ? (LedRgb){64u, 0u, 0u} : (LedRgb){0u, 0u, 0u};
    } else if (low_battery) {
        color = pulse_on(now_ms % 5000u, 200u, 400u, 3u)
            ? (LedRgb){64u, 8u, 0u} : (LedRgb){0u, 0u, 0u};
    } else {
        return false;
    }

    for (size_t i = 0u; i < 4u; ++i) {
        frame->pixels[i] = color;
    }
    return true;
}

static void render_lighting_loss(uint32_t now_ms, LightingFrame *frame)
{
    const LedRgb color = pulse_on(now_ms % 2000u, 100u, 200u, 2u)
        ? (LedRgb){32u, 8u, 0u} : (LedRgb){0u, 0u, 0u};
    for (size_t i = 0u; i < 4u; ++i) {
        frame->pixels[i] = color;
    }
}

void lighting_controller_init(LightingController *controller)
{
    if (controller == NULL) {
        return;
    }
    memset(controller, 0, sizeof *controller);
    controller->roof_mode = ROOF_LIGHT_OFF;
}

void lighting_controller_render(LightingController *controller,
                                uint32_t now_ms,
                                const VehicleState *state,
                                bool low_battery,
                                bool board_fault,
                                LightingFrame *frame,
                                size_t pixel_count)
{
    if (frame == NULL) {
        return;
    }

    memset(frame, 0, sizeof *frame);
    frame->roof_mode = ROOF_LIGHT_OFF;
    if (controller == NULL || state == NULL || pixel_count < 4u) {
        return;
    }
    if (pixel_count > LED_MAX_PIXELS) {
        pixel_count = LED_MAX_PIXELS;
    }

    if (state->link != LINK_OK || !state->lighting_rc_valid) {
        if (controller->has_seen_valid_lighting_rc) {
            render_lighting_loss(now_ms, frame);
            led_limit_current(frame->pixels, pixel_count,
                              LED_CURRENT_BUDGET_MA);
            frame->estimated_ma = led_estimated_ma(frame->pixels,
                                                   pixel_count);
        }
        return;
    }
    controller->has_seen_valid_lighting_rc = true;

    frame->front_duty = normalized_to_duty(state->aux6);
    frame->roof_spot_duty = normalized_to_duty(state->aux7);
    update_roof_mode(controller, state->aux8);
    update_brake(controller, now_ms, state->throttle);
    frame->roof_mode = controller->roof_mode;
    render_rear(controller, now_ms, state, frame);
    if (!render_warning(now_ms, low_battery, board_fault, frame)) {
        render_roof(controller, now_ms, state, frame, pixel_count);
    }
    led_limit_current(frame->pixels, pixel_count, LED_CURRENT_BUDGET_MA);
    frame->estimated_ma = led_estimated_ma(frame->pixels, pixel_count);
}
