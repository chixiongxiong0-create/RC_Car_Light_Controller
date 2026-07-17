#include "ui/screen_showcase.h"

enum {
    SHOWCASE_IDLE_MS = 4000u,
    SHOWCASE_ARMED_MS = 1500u,
    PAGE_SHUTTER_MS = 100u
};

static uint32_t elapsed(uint32_t now_ms, uint32_t epoch_ms)
{
    return (uint32_t)(now_ms - epoch_ms);
}

void showcase_model_init(ShowcaseModel *model, uint32_t now_ms, bool armed)
{
    if (model == 0) {
        return;
    }
    model->scene = SHOW_LOGO;
    model->stripe_direction = SHOW_STRIPE_CENTER;
    model->idle_epoch_ms = now_ms;
    model->armed_epoch_ms = now_ms;
    model->armed_hold = false;
    model->was_armed = armed;
}

ShowcaseModel showcase_model_update(ShowcaseModel model,
                                    const VehicleState *state,
                                    uint32_t now_ms)
{
    if (state == 0) {
        return model;
    }

    if (state->armed && !model.was_armed) {
        model.armed_epoch_ms = now_ms;
        model.armed_hold = true;
    }
    model.was_armed = state->armed;

    if (model.armed_hold && elapsed(now_ms, model.armed_epoch_ms) >= SHOWCASE_ARMED_MS) {
        model.armed_hold = false;
        model.idle_epoch_ms = now_ms;
    }

    if (model.armed_hold) {
        model.scene = SHOW_ARMED;
    } else if (state->throttle > 0.2f || state->throttle < -0.2f) {
        model.scene = SHOW_MOTION;
    } else {
        const uint32_t periods = elapsed(now_ms, model.idle_epoch_ms) / SHOWCASE_IDLE_MS;
        model.scene = (periods & 1u) == 0u ? SHOW_LOGO : SHOW_SLOGAN;
    }

    if (state->steering < 0.0f) {
        model.stripe_direction = SHOW_STRIPE_LEFT;
    } else if (state->steering > 0.0f) {
        model.stripe_direction = SHOW_STRIPE_RIGHT;
    } else {
        model.stripe_direction = SHOW_STRIPE_CENTER;
    }
    return model;
}

void page_transition_init(PageTransition *transition, UiPage page,
                          uint32_t now_ms)
{
    transition->visible_page = page;
    transition->requested_page = page;
    transition->phase = PAGE_TRANSITION_IDLE;
    transition->phase_epoch_ms = now_ms;
    transition->phase_start_cover = 0u;
}

void page_transition_request(PageTransition *transition, UiPage page,
                             uint32_t now_ms)
{
    const uint16_t cover = page_transition_cover_permille(transition, now_ms);
    if (page > UI_PAGE_SHOWCASE) {
        return;
    }
    transition->requested_page = page;
    if (page == transition->visible_page) {
        if (transition->phase == PAGE_TRANSITION_COVER) {
            transition->phase = PAGE_TRANSITION_RETRACT;
            transition->phase_epoch_ms = now_ms;
            transition->phase_start_cover = cover;
        }
        return;
    }
    if (transition->phase != PAGE_TRANSITION_COVER) {
        transition->phase = PAGE_TRANSITION_COVER;
        transition->phase_epoch_ms = now_ms;
        transition->phase_start_cover = cover;
    }
}

void page_transition_tick(PageTransition *transition, uint32_t now_ms)
{
    if (transition->phase == PAGE_TRANSITION_COVER &&
        elapsed(now_ms, transition->phase_epoch_ms) >= PAGE_SHUTTER_MS) {
        transition->visible_page = transition->requested_page;
        transition->phase = PAGE_TRANSITION_RETRACT;
        transition->phase_epoch_ms = now_ms;
        transition->phase_start_cover = 1000u;
    } else if (transition->phase == PAGE_TRANSITION_RETRACT &&
               elapsed(now_ms, transition->phase_epoch_ms) >= PAGE_SHUTTER_MS) {
        transition->phase = PAGE_TRANSITION_IDLE;
        transition->phase_epoch_ms = now_ms;
        transition->phase_start_cover = 0u;
        if (transition->requested_page != transition->visible_page) {
            transition->phase = PAGE_TRANSITION_COVER;
        }
    }
}

UiPage page_transition_visible_page(const PageTransition *transition)
{
    return transition->visible_page;
}

PageTransitionPhase page_transition_phase(const PageTransition *transition)
{
    return transition->phase;
}

uint16_t page_transition_cover_permille(const PageTransition *transition,
                                        uint32_t now_ms)
{
    uint32_t amount;
    uint32_t cover;
    if (transition->phase == PAGE_TRANSITION_IDLE) {
        return 0u;
    }
    amount = elapsed(now_ms, transition->phase_epoch_ms);
    if (amount > PAGE_SHUTTER_MS) {
        amount = PAGE_SHUTTER_MS;
    }
    if (transition->phase == PAGE_TRANSITION_COVER) {
        cover = transition->phase_start_cover +
                ((1000u - transition->phase_start_cover) * amount) / PAGE_SHUTTER_MS;
    } else {
        cover = transition->phase_start_cover -
                (transition->phase_start_cover * amount) / PAGE_SHUTTER_MS;
    }
    return (uint16_t)cover;
}
