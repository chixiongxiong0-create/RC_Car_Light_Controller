#include "lighting/lighting_controller.h"

#include <string.h>

static float clamp_normalized(float value)
{
    if (value < -1.0f) return -1.0f;
    if (value > 1.0f) return 1.0f;
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
    if (mode >= (unsigned)ROOF_LIGHT_MODE_COUNT) mode = (unsigned)ROOF_LIGHT_STATUS;
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
        if (value >= boundary + hysteresis) controller->roof_mode = candidate;
    } else if (candidate < controller->roof_mode) {
        const float boundary = -1.0f +
            0.25f * (float)(unsigned)controller->roof_mode;
        if (value <= boundary - hysteresis) controller->roof_mode = candidate;
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

static LedRgb *roof_pixel(Ws2812Frame *frame, size_t index)
{
    return &frame->groups[2u + index / 8u][index % 8u];
}

static void render_roof(const LightingController *controller, uint32_t now_ms,
                        const VehicleState *state, Ws2812Frame *frame)
{
    const size_t roof_count = 16u;
    const uint8_t level = normalized_to_level(state->aux9);
    const uint32_t period = animation_period(state->aux9);

    if (controller->roof_mode == ROOF_LIGHT_OFF) return;
    if (controller->roof_mode == ROOF_LIGHT_STEADY_WHITE) {
        const LedRgb color = {level, level, level};
        for (size_t i = 0u; i < roof_count; ++i) *roof_pixel(frame, i) = color;
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_WARM_TRAIL) {
        const LedRgb color = scale_color((LedRgb){96u, 48u, 8u}, level);
        for (size_t i = 0u; i < roof_count; ++i) *roof_pixel(frame, i) = color;
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
        for (size_t i = 0u; i < roof_count; ++i) *roof_pixel(frame, i) = color;
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_COMET) {
        const size_t head = ((now_ms % period) * roof_count) / period;
        const size_t tail = (head + roof_count - 1u) % roof_count;
        *roof_pixel(frame, head) = (LedRgb){96u, 48u, 8u};
        *roof_pixel(frame, tail) = (LedRgb){24u, 8u, 1u};
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_RAINBOW) {
        const uint32_t phase = ((now_ms % period) * 256u) / period;
        for (size_t i = 0u; i < roof_count; ++i) {
            const uint32_t offset = (i * 256u) / roof_count;
            *roof_pixel(frame, i) = rainbow_color((uint8_t)(phase + offset));
        }
        return;
    }
    if (controller->roof_mode == ROOF_LIGHT_POLICE) {
        const bool swapped = ((now_ms / (period / 2u)) & 1u) != 0u;
        for (size_t i = 0u; i < roof_count; ++i) {
            const bool red = ((i & 1u) == 0u) != swapped;
            *roof_pixel(frame, i) = red
                ? (LedRgb){96u, 0u, 0u} : (LedRgb){0u, 0u, 96u};
        }
        return;
    }

    LedRgb status;
    if (!state->battery_valid) status = (LedRgb){0u, 0u, level};
    else if (state->battery_v < 7.0f) status = (LedRgb){level, (uint8_t)(level / 3u), 0u};
    else status = (LedRgb){0u, level, 0u};
    for (size_t i = 0u; i < roof_count; ++i) *roof_pixel(frame, i) = status;
}

static void update_brake(LightingController *controller, uint32_t now_ms,
                         float throttle)
{
    if (controller->brake_active &&
        (uint32_t)(now_ms - controller->brake_trigger_ms) > 600u) {
        controller->brake_active = false;
    }
    bool triggered = false;
    if (controller->forward_armed && controller->has_previous_throttle &&
        ((controller->previous_throttle - throttle) >= 0.15f || throttle <= 0.10f)) {
        controller->brake_active = true;
        controller->brake_trigger_ms = now_ms;
        controller->forward_armed = false;
        triggered = true;
    }
    if (!triggered && throttle > 0.25f) controller->forward_armed = true;
    controller->previous_throttle = throttle;
    controller->has_previous_throttle = true;
}

static void copy_rear(Ws2812Frame *frame, const LedRgb rear[4])
{
    for (size_t i = 0u; i < 4u; ++i) {
        frame->groups[0][i] = rear[i];
        frame->groups[1][i] = rear[3u - i];
    }
}

static void render_rear(const LightingController *controller, uint32_t now_ms,
                        const VehicleState *state, Ws2812Frame *frame)
{
    const LedRgb base = {12u, 0u, 0u};
    const LedRgb brake = {96u, 0u, 0u};
    const LedRgb reverse_color = {96u, 96u, 96u};
    const LedRgb amber = {96u, 32u, 0u};
    LedRgb rear[4] = {base, base, base, base};

    if (controller->brake_active) {
        for (size_t i = 0u; i < 4u; ++i) rear[i] = brake;
        copy_rear(frame, rear);
        return;
    }
    const bool reverse = state->throttle < -0.20f;
    if (reverse) {
        rear[1] = reverse_color;
        rear[2] = reverse_color;
    }
    if ((now_ms % 666u) < 333u) {
        if (state->steering < -0.30f) {
            rear[0] = amber;
            if (!reverse) rear[1] = amber;
        } else if (state->steering > 0.30f) {
            if (!reverse) rear[2] = amber;
            rear[3] = amber;
        }
    }
    copy_rear(frame, rear);
}

static bool pulse_on(uint32_t phase_ms, uint32_t on_ms,
                     uint32_t spacing_ms, unsigned pulses)
{
    for (unsigned i = 0u; i < pulses; ++i) {
        const uint32_t start = i * spacing_ms;
        if (phase_ms >= start && phase_ms < start + on_ms) return true;
    }
    return false;
}

static bool render_warning(uint32_t now_ms, bool low_battery, bool board_fault,
                           Ws2812Frame *frame)
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
    const LedRgb rear[4] = {color, color, color, color};
    copy_rear(frame, rear);
    return true;
}

static void render_lighting_loss(uint32_t now_ms, Ws2812Frame *frame)
{
    const LedRgb color = pulse_on(now_ms % 2000u, 100u, 200u, 2u)
        ? (LedRgb){32u, 8u, 0u} : (LedRgb){0u, 0u, 0u};
    const LedRgb rear[4] = {color, color, color, color};
    copy_rear(frame, rear);
}

static void limit_frame(LightingFrame *frame)
{
    LedRgb flat[WS2812_TOTAL_PIXELS];
    size_t offset = 0u;
    for (size_t group = 0u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            flat[offset++] = frame->ws2812.groups[group][pixel];
        }
    }
    led_limit_current(flat, WS2812_TOTAL_PIXELS, 1000u);
    offset = 0u;
    for (size_t group = 0u; group < WS2812_GROUP_COUNT; ++group) {
        for (size_t pixel = 0u; pixel < WS2812_GROUP_LENGTHS[group]; ++pixel) {
            frame->ws2812.groups[group][pixel] = flat[offset++];
        }
    }
    frame->estimated_ma = led_estimated_ma(flat, WS2812_TOTAL_PIXELS);
}

void lighting_controller_init(LightingController *controller)
{
    if (controller == NULL) return;
    memset(controller, 0, sizeof *controller);
    controller->roof_mode = ROOF_LIGHT_OFF;
}

void lighting_controller_render(LightingController *controller, uint32_t now_ms,
                                const VehicleState *state, bool low_battery,
                                bool board_fault, LightingFrame *frame)
{
    if (frame == NULL) return;
    memset(frame, 0, sizeof *frame);
    frame->roof_mode = ROOF_LIGHT_OFF;
    if (controller == NULL || state == NULL) return;

    if (state->link != LINK_OK || !state->lighting_rc_valid) {
        if (controller->has_seen_valid_lighting_rc) {
            render_lighting_loss(now_ms, &frame->ws2812);
            limit_frame(frame);
        }
        return;
    }
    controller->has_seen_valid_lighting_rc = true;
    frame->front_duty = normalized_to_duty(state->aux6);
    frame->roof_spot_duty = normalized_to_duty(state->aux7);
    update_roof_mode(controller, state->aux8);
    update_brake(controller, now_ms, state->throttle);
    frame->roof_mode = controller->roof_mode;
    render_rear(controller, now_ms, state, &frame->ws2812);
    if (!render_warning(now_ms, low_battery, board_fault, &frame->ws2812)) {
        render_roof(controller, now_ms, state, &frame->ws2812);
    }
    limit_frame(frame);
}
