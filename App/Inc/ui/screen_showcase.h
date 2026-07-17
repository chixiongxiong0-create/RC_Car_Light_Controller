#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui/ui_types.h"
#include "vehicle_state.h"

typedef struct _lv_obj_t lv_obj_t;

typedef enum {
    SHOW_LOGO,
    SHOW_SLOGAN,
    SHOW_ARMED,
    SHOW_MOTION
} ShowcaseScene;

typedef enum {
    SHOW_STRIPE_CENTER,
    SHOW_STRIPE_LEFT,
    SHOW_STRIPE_RIGHT
} ShowcaseStripeDirection;

typedef struct {
    ShowcaseScene scene;
    ShowcaseStripeDirection stripe_direction;
    uint32_t idle_epoch_ms;
    uint32_t armed_epoch_ms;
    bool armed_hold;
    bool was_armed;
} ShowcaseModel;

void showcase_model_init(ShowcaseModel *model, uint32_t now_ms, bool armed);
ShowcaseModel showcase_model_update(ShowcaseModel previous,
                                    const VehicleState *state,
                                    uint32_t now_ms);

typedef enum {
    PAGE_TRANSITION_IDLE,
    PAGE_TRANSITION_COVER,
    PAGE_TRANSITION_RETRACT
} PageTransitionPhase;

typedef struct {
    UiPage visible_page;
    UiPage requested_page;
    PageTransitionPhase phase;
    uint32_t phase_epoch_ms;
} PageTransition;

void page_transition_init(PageTransition *transition, UiPage page,
                          uint32_t now_ms);
void page_transition_request(PageTransition *transition, UiPage page,
                             uint32_t now_ms);
void page_transition_tick(PageTransition *transition, uint32_t now_ms);
UiPage page_transition_visible_page(const PageTransition *transition);
PageTransitionPhase page_transition_phase(const PageTransition *transition);
uint16_t page_transition_cover_permille(const PageTransition *transition,
                                        uint32_t now_ms);

lv_obj_t *screen_showcase_create(void);
void screen_showcase_update(uint32_t now_ms, const VehicleState *state);
