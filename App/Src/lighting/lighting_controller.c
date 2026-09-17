#include "lighting/lighting_controller.h"

#include <string.h>

#if (LIGHTING_LEFT_STRIP_GROUP < 2u || LIGHTING_LEFT_STRIP_GROUP > 3u || \
     LIGHTING_RIGHT_STRIP_GROUP < 2u || LIGHTING_RIGHT_STRIP_GROUP > 3u || \
     LIGHTING_LEFT_STRIP_GROUP == LIGHTING_RIGHT_STRIP_GROUP || \
     LIGHTING_LEFT_STRIP_REVERSED > 1u || LIGHTING_RIGHT_STRIP_REVERSED > 1u)
#error Invalid drive-sync strip mapping
#endif

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
    return (uint8_t)(unit * 200.0f + 0.5f);
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

static LedRgb *drive_pixel(Ws2812Frame *frame, bool right, size_t logical_index)
{
    const size_t group = right ? LIGHTING_RIGHT_STRIP_GROUP : LIGHTING_LEFT_STRIP_GROUP;
    const bool reversed = right ? LIGHTING_RIGHT_STRIP_REVERSED : LIGHTING_LEFT_STRIP_REVERSED;
    return &frame->groups[group][reversed ? 7u - logical_index : logical_index];
}

static LedRgb drive_scale(LedRgb color, uint8_t level)
{
    color.r = (uint8_t)(((uint16_t)color.r * level) / 200u);
    color.g = (uint8_t)(((uint16_t)color.g * level) / 200u);
    color.b = (uint8_t)(((uint16_t)color.b * level) / 200u);
    return color;
}

static void render_drive_sync(const LightingController *controller, uint32_t now_ms,
                              const VehicleState *state, Ws2812Frame *frame,
                              uint8_t level)
{
    const bool reverse = state->throttle < -0.20f;
    const bool forward = state->throttle > 0.12f;
    const uint32_t brake_age = now_ms - controller->brake_trigger_ms;
    for (size_t side = 0u; side < 2u; ++side) {
        const bool turn_side = controller->active_turn == (side == 0u ? -1 : 1);
        const uint32_t turn_phase = (now_ms - controller->turn_start_ms) % 666u;
        const size_t turn_lit = turn_phase < 333u ? 1u + turn_phase / 42u : 0u;
        for (size_t i = 0u; i < 8u; ++i) {
            LedRgb color = {36u, 0u, 0u};
            bool reverse_white = false;
            if (controller->brake_active) {
                const size_t distance = i < 4u ? i : 7u - i;
                const uint32_t wave = brake_age < 160u ? brake_age : brake_age - 160u;
                color.r = brake_age >= 320u || distance == (wave / 40u) % 4u
                    ? 200u : 130u;
            } else if (reverse) {
                const size_t head = 7u - ((now_ms / 110u) % 8u);
                reverse_white = i == head || i == (head == 7u ? 6u : head + 1u);
                if (reverse_white) color = (LedRgb){180u, 180u, 180u};
            } else if (forward) {
                const float throttle = clamp_normalized(state->throttle);
                const size_t fill = 1u + (size_t)(throttle * 7.0f);
                if (i < fill) color = (LedRgb){145u, 42u, 0u};
                const uint32_t step_ms = 180u - (uint32_t)(throttle * 100.0f);
                const size_t head = (now_ms / step_ms) % fill;
                if (i == head) color = (LedRgb){180u, 86u, 12u};
            } else {
                const uint32_t phase = (now_ms / 80u) % 8u;
                const size_t radius = phase < 4u ? phase : 7u - phase;
                const size_t distance = i < 4u ? 3u - i : i - 4u;
                if (distance == radius) color.r = 92u;
            }
            if (turn_side && i < turn_lit && !reverse_white) {
                color = (LedRgb){200u, 75u, 0u};
            }
            *drive_pixel(frame, side != 0u, i) = drive_scale(color, level);
        }
    }
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
    if (controller->roof_mode == ROOF_LIGHT_DRIVE_SYNC) {
        render_drive_sync(controller, now_ms, state, frame, level);
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

static void update_turn(LightingController *controller, uint32_t now_ms,
                        float steering)
{
    const int8_t turn = steering < -0.30f ? -1 : steering > 0.30f ? 1 : 0;
    if (turn != controller->active_turn) {
        controller->active_turn = turn;
        controller->turn_start_ms = now_ms;
    }
}

static void copy_rear(Ws2812Frame *frame, const LedRgb rear[4])
{
    for (size_t i = 0u; i < 4u; ++i) {
        frame->groups[0][i] = rear[i];
        frame->groups[1][i] = rear[i];
    }
}

static void copy_rear_sides(Ws2812Frame *frame, const LedRgb left[4],
                            const LedRgb right[4])
{
    for (size_t i = 0u; i < 4u; ++i) {
        frame->groups[0][i] = left[i];
        frame->groups[1][i] = right[i];
    }
}

static uint8_t reverse_white_level(uint32_t now_ms)
{
    const uint32_t phase = now_ms % 800u;
    if (phase < 160u || phase >= 240u) return 160u;
    if (phase < 200u) return (uint8_t)(160u + phase - 160u);
    return (uint8_t)(200u - (phase - 200u));
}

static uint8_t brake_red_level(const LightingController *controller,
                               uint32_t now_ms)
{
    const uint32_t age = now_ms - controller->brake_trigger_ms;
    if ((age >= 90u && age < 160u) || (age >= 250u && age < 320u)) {
        return 150u;
    }
    return 200u;
}

static void render_rear(const LightingController *controller, uint32_t now_ms,
                        const VehicleState *state, Ws2812Frame *frame)
{
    const LedRgb base = {40u, 0u, 0u};
    const LedRgb amber = {200u, 70u, 0u};
    LedRgb left[4] = {base, base, base, base};
    LedRgb right[4] = {base, base, base, base};
    const bool reverse = state->throttle < -0.20f;

    if (controller->brake_active) {
        const LedRgb brake = {brake_red_level(controller, now_ms), 0u, 0u};
        for (size_t i = 0u; i < 4u; ++i) left[i] = right[i] = brake;
    } else if (reverse) {
        const uint8_t level = reverse_white_level(now_ms);
        const LedRgb white = {level, level, level};
        left[1] = right[1] = white;
        left[2] = right[2] = white;
    } else {
        const size_t head = (now_ms / 120u) % 4u;
        const size_t tail = (head + 3u) % 4u;
        left[head] = right[head] = (LedRgb){120u, 0u, 0u};
        left[tail] = right[tail] = (LedRgb){70u, 0u, 0u};
    }

    if (controller->active_turn != 0) {
        const uint32_t turn_phase =
            (now_ms - controller->turn_start_ms) % 666u;
        if (turn_phase < 333u) {
            LedRgb *turn_side = controller->active_turn < 0 ? left : right;
            size_t lit = turn_phase / 80u + 1u;
            if (lit > 4u) lit = 4u;
            for (size_t i = 0u; i < lit; ++i) {
                if (reverse && !controller->brake_active && (i == 1u || i == 2u)) {
                    continue;
                }
                turn_side[i] = amber;
            }
        }
    }
    copy_rear_sides(frame, left, right);
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
    led_limit_current(flat, WS2812_TOTAL_PIXELS,
                      LIGHTING_ESTIMATED_CURRENT_BUDGET_MA);
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
        controller->active_turn = 0;
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
    update_turn(controller, now_ms, state->steering);
    frame->roof_mode = controller->roof_mode;
    render_rear(controller, now_ms, state, &frame->ws2812);
    if (!render_warning(now_ms, low_battery, board_fault, &frame->ws2812)) {
        render_roof(controller, now_ms, state, &frame->ws2812);
    }
    limit_frame(frame);
}
