#include "lighting/f469_vehicle_effects.h"

enum { RAINBOW_PERIOD_MS = 5000u, TURN_PERIOD_MS = 666u,
       BRAKE_BREATH_MS = 400u };

/* Logical side pixel 0 is at the front; pixel 7 is at the rear. */
static LedRgb *side_pixel(Ws2812Frame *frame, unsigned side, unsigned index)
{
    const unsigned group = side == 0u ? LIGHTING_LEFT_STRIP_GROUP :
                                         LIGHTING_RIGHT_STRIP_GROUP;
    const bool reversed = side == 0u ? LIGHTING_LEFT_STRIP_REVERSED != 0u :
                                        LIGHTING_RIGHT_STRIP_REVERSED != 0u;
    return &frame->groups[group][reversed ? 7u - index : index];
}

static uint8_t smooth_breath(uint32_t phase, uint32_t period,
                             uint8_t minimum, uint8_t maximum)
{
    const uint32_t half = period / 2u;
    const uint32_t distance = phase < half ? phase : period - phase;
    const uint32_t x = distance * 255u / half;
    const uint32_t eased = x * x * (765u - 2u * x) / 65025u;
    return (uint8_t)(minimum + (uint32_t)(maximum - minimum) * eased / 255u);
}

static LedRgb rainbow(uint32_t phase, uint8_t level)
{
    const uint32_t hue = phase * 1536u / RAINBOW_PERIOD_MS;
    const uint8_t slope = (uint8_t)(hue % 256u);
    const uint8_t up = (uint8_t)((uint32_t)level * slope / 255u);
    const uint8_t down = (uint8_t)(level - up);
    switch (hue / 256u) {
    case 0u: return (LedRgb){level, up, 0u};
    case 1u: return (LedRgb){down, level, 0u};
    case 2u: return (LedRgb){0u, level, up};
    case 3u: return (LedRgb){0u, down, level};
    case 4u: return (LedRgb){up, 0u, level};
    default: return (LedRgb){level, 0u, down};
    }
}

static LedRgb scaled(LedRgb color, uint8_t level, uint8_t maximum)
{
    color.r = (uint8_t)((uint32_t)color.r * level / maximum);
    color.g = (uint8_t)((uint32_t)color.g * level / maximum);
    color.b = (uint8_t)((uint32_t)color.b * level / maximum);
    return color;
}

static uint32_t color_step(LightingController *controller, uint32_t now_ms,
                           float throttle)
{
    if (throttle < 0.0f) throttle = -throttle;
    if (throttle > 1.0f) throttle = 1.0f;
    const uint32_t steps_per_mille =
        3u * (1000u + (uint32_t)(throttle * 10000.0f + 0.5f));
    if (controller->color_clock_started) {
        const uint32_t elapsed = now_ms - controller->color_last_ms;
        if (elapsed < 1000u)
            controller->color_phase_milli =
                (controller->color_phase_milli + elapsed * steps_per_mille / 1000u) % 8000u;
    } else {
        controller->color_clock_started = true;
    }
    controller->color_last_ms = now_ms;
    return controller->color_phase_milli / 1000u;
}

static void render_stopped(uint32_t now_ms, Ws2812Frame *frame)
{
    const uint32_t time_phase = now_ms % RAINBOW_PERIOD_MS;
    for (unsigned side = 0u; side < 2u; ++side) {
        for (unsigned i = 0u; i < 4u; ++i) {
            const uint32_t phase = (time_phase + i * 1250u) % RAINBOW_PERIOD_MS;
            const uint8_t level = smooth_breath(phase, RAINBOW_PERIOD_MS, 32u, 160u);
            frame->groups[side][i] = rainbow(phase, level);
        }
        for (unsigned i = 0u; i < 8u; ++i) {
            const uint32_t phase = (time_phase + i * 625u) % RAINBOW_PERIOD_MS;
            const uint8_t level = smooth_breath(phase, RAINBOW_PERIOD_MS, 32u, 160u);
            *side_pixel(frame, side, i) = rainbow(phase, level);
        }
    }
}

static void render_forward(uint32_t step, Ws2812Frame *frame)
{
    static const LedRgb tail[4] = {
        {160u, 0u, 0u}, {0u, 0u, 160u},
        {0u, 160u, 0u}, {160u, 120u, 0u}
    };
    static const LedRgb side_colors[8] = {
        {160u, 0u, 0u}, {160u, 70u, 0u}, {160u, 140u, 0u},
        {0u, 160u, 0u}, {0u, 160u, 160u}, {0u, 0u, 160u},
        {80u, 0u, 160u}, {160u, 0u, 120u}
    };
    for (unsigned side = 0u; side < 2u; ++side) {
        for (unsigned i = 0u; i < 4u; ++i)
            frame->groups[side][i] = tail[(i + step) % 4u];
        for (unsigned i = 0u; i < 8u; ++i) {
            const uint8_t brightness = (uint8_t)(40u + (7u - i) * 120u / 7u);
            *side_pixel(frame, side, i) =
                scaled(side_colors[(i + step) % 8u], brightness, 160u);
        }
    }
}

static void render_reverse(uint32_t step, Ws2812Frame *frame)
{
    static const LedRgb near_white[8] = {
        {200u, 200u, 200u}, {200u, 192u, 180u},
        {180u, 200u, 196u}, {196u, 190u, 200u},
        {200u, 200u, 180u}, {180u, 196u, 200u},
        {200u, 184u, 196u}, {192u, 200u, 200u}
    };
    step %= 8u;
    for (unsigned side = 0u; side < 2u; ++side) {
        for (unsigned i = 0u; i < 4u; ++i)
            frame->groups[side][i] = (LedRgb){160u, 160u, 160u};
        for (unsigned i = 0u; i < 8u; ++i) {
            const uint8_t brightness = (uint8_t)(40u + i * 120u / 7u);
            *side_pixel(frame, side, i) =
                scaled(near_white[(i + 8u - step) % 8u], brightness, 200u);
        }
    }
}

static void render_brake(uint32_t now_ms, Ws2812Frame *frame)
{
    const uint8_t brightness =
        smooth_breath(now_ms % BRAKE_BREATH_MS, BRAKE_BREATH_MS, 48u, 200u);
    for (unsigned side = 0u; side < 2u; ++side) {
        for (unsigned i = 0u; i < 4u; ++i)
            frame->groups[side][i] = (LedRgb){200u, 0u, 0u};
        for (unsigned i = 0u; i < 8u; ++i)
            *side_pixel(frame, side, i) = (LedRgb){brightness, 0u, 0u};
    }
}

static void overlay_turn(const LightingController *controller, uint32_t now_ms,
                         Ws2812Frame *frame)
{
    if (controller->active_turn == 0 ||
        (now_ms - controller->turn_start_ms) % TURN_PERIOD_MS >= TURN_PERIOD_MS / 2u)
        return;
    const unsigned side = controller->active_turn < 0 ? 0u : 1u;
    const LedRgb amber = {200u, 120u, 0u};
    for (unsigned i = 0u; i < 4u; ++i) frame->groups[side][i] = amber;
    for (unsigned i = 0u; i < 8u; ++i) *side_pixel(frame, side, i) = amber;
}

void f469_vehicle_effects_render(LightingController *controller,
                                 uint32_t now_ms, const VehicleState *state,
                                 Ws2812Frame *frame)
{
    const uint32_t step = color_step(controller, now_ms, state->throttle);
    if (controller->brake_active) render_brake(now_ms, frame);
    else if (state->throttle < -0.12f) render_reverse(step, frame);
    else if (state->throttle > 0.12f) render_forward(step, frame);
    else render_stopped(now_ms, frame);
    overlay_turn(controller, now_ms, frame);
}
