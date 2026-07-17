#include "input_manager.h"

#include <stddef.h>

enum {
    BUTTON_DEBOUNCE_MS = 30u,
    BUTTON_CYCLE_MAX_MS = 800u,
    BUTTON_BRIGHTNESS_MIN_MS = 2000u
};

typedef enum {
    AUX_ZONE_UNKNOWN,
    AUX_ZONE_LOW,
    AUX_ZONE_MID,
    AUX_ZONE_HIGH
} AuxZone;

static UiPage current_page;
static AuxZone current_zone;
static bool raw_button;
static bool stable_button;
static uint32_t raw_changed_ms;
static uint32_t press_started_ms;
static bool touch_available;
static bool touch_pending;
static UiPage touch_page;
static bool brightness_requested;

static AuxZone aux_zone(float value)
{
    switch (current_zone) {
    case AUX_ZONE_LOW:
        if (value >= 0.35f) {
            return AUX_ZONE_HIGH;
        }
        return value > -0.27f ? AUX_ZONE_MID : AUX_ZONE_LOW;
    case AUX_ZONE_HIGH:
        if (value <= -0.35f) {
            return AUX_ZONE_LOW;
        }
        return value < 0.27f ? AUX_ZONE_MID : AUX_ZONE_HIGH;
    case AUX_ZONE_MID:
        if (value <= -0.35f) {
            return AUX_ZONE_LOW;
        }
        if (value >= 0.35f) {
            return AUX_ZONE_HIGH;
        }
        return AUX_ZONE_MID;
    default:
        if (value <= -0.35f) {
            return AUX_ZONE_LOW;
        }
        if (value >= 0.35f) {
            return AUX_ZONE_HIGH;
        }
        return AUX_ZONE_MID;
    }
}

static UiPage page_for_zone(AuxZone zone)
{
    if (zone == AUX_ZONE_LOW) {
        return UI_PAGE_DASHBOARD;
    }
    if (zone == AUX_ZONE_HIGH) {
        return UI_PAGE_SHOWCASE;
    }
    return UI_PAGE_FACE;
}

void input_manager_init(void)
{
    current_page = UI_PAGE_DASHBOARD;
    current_zone = AUX_ZONE_UNKNOWN;
    raw_button = false;
    stable_button = false;
    raw_changed_ms = 0u;
    press_started_ms = 0u;
    touch_available = false;
    touch_pending = false;
    touch_page = UI_PAGE_DASHBOARD;
    brightness_requested = false;
}

void input_manager_set_button(bool pressed, uint32_t now_ms)
{
    if (pressed != raw_button) {
        raw_button = pressed;
        raw_changed_ms = now_ms;
    }
}

void input_manager_set_touch_available(bool available)
{
    touch_available = available;
    if (!available) {
        touch_pending = false;
    }
}

void input_manager_set_touch_page(UiPage page)
{
    if (touch_available && page <= UI_PAGE_SHOWCASE) {
        touch_page = page;
        touch_pending = true;
    }
}

void input_manager_tick(uint32_t now_ms, const VehicleState *state)
{
    if (raw_button != stable_button &&
        (uint32_t)(now_ms - raw_changed_ms) >= BUTTON_DEBOUNCE_MS) {
        stable_button = raw_button;
        if (stable_button) {
            press_started_ms = raw_changed_ms;
        } else {
            const uint32_t duration_ms = (uint32_t)(raw_changed_ms - press_started_ms);
            if (duration_ms >= BUTTON_BRIGHTNESS_MIN_MS) {
                brightness_requested = true;
            } else if (duration_ms >= BUTTON_DEBOUNCE_MS &&
                       duration_ms <= BUTTON_CYCLE_MAX_MS) {
                current_page = (UiPage)(((unsigned)current_page + 1u) % 3u);
            }
        }
    }

    if (touch_pending) {
        current_page = touch_page;
        touch_pending = false;
    }

    if (state != NULL) {
        const AuxZone next_zone = aux_zone(state->aux_page);
        if (next_zone != current_zone) {
            current_zone = next_zone;
            current_page = page_for_zone(next_zone);
        }
    }
}

UiPage input_manager_page(void)
{
    return current_page;
}

bool input_manager_take_brightness_request(void)
{
    const bool result = brightness_requested;
    brightness_requested = false;
    return result;
}
