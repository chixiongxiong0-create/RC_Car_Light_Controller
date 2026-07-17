#include <assert.h>
#include <stdint.h>

#include "ui/screen_showcase.h"

static VehicleState vehicle(float throttle, float steering, bool armed)
{
    VehicleState state = {0};
    state.throttle = throttle;
    state.steering = steering;
    state.armed = armed;
    state.link = LINK_OK;
    return state;
}

static void test_idle_rotation(void)
{
    ShowcaseModel model;
    showcase_model_init(&model, 100u, false);
    assert(showcase_model_update(model, &(VehicleState){0}, 4099u).scene == SHOW_LOGO);
    model = showcase_model_update(model, &(VehicleState){0}, 4100u);
    assert(model.scene == SHOW_SLOGAN);
    assert(showcase_model_update(model, &(VehicleState){0}, 8099u).scene == SHOW_SLOGAN);
    model = showcase_model_update(model, &(VehicleState){0}, 8100u);
    assert(model.scene == SHOW_LOGO);
}

static void test_armed_edge_and_motion_priority(void)
{
    ShowcaseModel model;
    VehicleState state = vehicle(0.0f, 0.0f, false);
    showcase_model_init(&model, 1000u, false);

    state.armed = true;
    model = showcase_model_update(model, &state, 1100u);
    assert(model.scene == SHOW_ARMED);
    model = showcase_model_update(model, &state, 2000u);
    assert(model.scene == SHOW_ARMED);
    model = showcase_model_update(model, &state, 2599u);
    assert(model.scene == SHOW_ARMED);
    model = showcase_model_update(model, &state, 2600u);
    assert(model.scene == SHOW_LOGO);

    state.throttle = 0.201f;
    state.steering = -0.1f;
    model = showcase_model_update(model, &state, 2700u);
    assert(model.scene == SHOW_MOTION);
    assert(model.stripe_direction == SHOW_STRIPE_LEFT);
    state.steering = 0.1f;
    model = showcase_model_update(model, &state, 2750u);
    assert(model.stripe_direction == SHOW_STRIPE_RIGHT);
    state.throttle = -0.2f;
    model = showcase_model_update(model, &state, 2800u);
    assert(model.scene != SHOW_MOTION);
}

static void test_repeated_armed_does_not_restart_hold(void)
{
    ShowcaseModel model;
    VehicleState state = vehicle(0.0f, 0.0f, false);
    showcase_model_init(&model, 0u, false);
    state.armed = true;
    model = showcase_model_update(model, &state, 10u);
    model = showcase_model_update(model, &state, 1000u);
    model = showcase_model_update(model, &state, 1509u);
    assert(model.scene == SHOW_ARMED);
    model = showcase_model_update(model, &state, 1510u);
    assert(model.scene != SHOW_ARMED);
}

static void test_tick_wrap(void)
{
    ShowcaseModel model;
    VehicleState state = vehicle(0.0f, 0.0f, false);
    showcase_model_init(&model, UINT32_MAX - 99u, false);
    assert(showcase_model_update(model, &state, 3899u).scene == SHOW_LOGO);
    model = showcase_model_update(model, &state, 3900u);
    assert(model.scene == SHOW_SLOGAN);

    showcase_model_init(&model, UINT32_MAX - 49u, false);
    state.armed = true;
    model = showcase_model_update(model, &state, UINT32_MAX - 9u);
    assert(showcase_model_update(model, &state, 1489u).scene == SHOW_ARMED);
    assert(showcase_model_update(model, &state, 1490u).scene != SHOW_ARMED);
}

static void test_transition_boundaries_and_latest_request(void)
{
    PageTransition transition;
    page_transition_init(&transition, UI_PAGE_DASHBOARD, 1000u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_DASHBOARD);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_IDLE);

    page_transition_request(&transition, UI_PAGE_FACE, 1010u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_COVER);
    assert(page_transition_visible_page(&transition) == UI_PAGE_DASHBOARD);
    assert(page_transition_cover_permille(&transition, 1059u) == 490u);
    page_transition_tick(&transition, 1109u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_DASHBOARD);
    page_transition_tick(&transition, 1110u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_RETRACT);
    assert(page_transition_visible_page(&transition) == UI_PAGE_FACE);
    assert(page_transition_cover_permille(&transition, 1110u) == 1000u);

    page_transition_request(&transition, UI_PAGE_SHOWCASE, 1150u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_FACE);
    page_transition_tick(&transition, 1209u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_COVER);
    page_transition_tick(&transition, 1250u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_SHOWCASE);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_RETRACT);
    page_transition_tick(&transition, 1350u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_IDLE);
    assert(page_transition_visible_page(&transition) == UI_PAGE_SHOWCASE);
    assert(page_transition_cover_permille(&transition, 1350u) == 0u);
}

static void test_transition_wrap_and_noop(void)
{
    PageTransition transition;
    page_transition_init(&transition, UI_PAGE_FACE, UINT32_MAX - 20u);
    page_transition_request(&transition, UI_PAGE_FACE, UINT32_MAX - 10u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_IDLE);
    page_transition_request(&transition, UI_PAGE_DASHBOARD, UINT32_MAX - 9u);
    page_transition_tick(&transition, 90u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_DASHBOARD);
    page_transition_tick(&transition, 190u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_IDLE);
}

static void test_transition_latest_cancel_and_continuity(void)
{
    PageTransition transition;
    page_transition_init(&transition, UI_PAGE_DASHBOARD, 0u);
    page_transition_request(&transition, UI_PAGE_FACE, 10u);
    page_transition_request(&transition, UI_PAGE_SHOWCASE, 60u);
    assert(page_transition_cover_permille(&transition, 60u) == 500u);
    page_transition_tick(&transition, 110u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_SHOWCASE);

    page_transition_tick(&transition, 150u);
    assert(page_transition_cover_permille(&transition, 150u) == 600u);
    page_transition_request(&transition, UI_PAGE_FACE, 150u);
    assert(page_transition_cover_permille(&transition, 150u) == 600u);
    assert(page_transition_cover_permille(&transition, 200u) == 800u);
    page_transition_tick(&transition, 250u);
    assert(page_transition_visible_page(&transition) == UI_PAGE_FACE);

    page_transition_tick(&transition, 290u);
    assert(page_transition_cover_permille(&transition, 290u) == 600u);
    page_transition_request(&transition, UI_PAGE_FACE, 290u);
    assert(page_transition_cover_permille(&transition, 290u) == 600u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_RETRACT);

    page_transition_init(&transition, UI_PAGE_DASHBOARD, 1000u);
    page_transition_request(&transition, UI_PAGE_FACE, 1010u);
    assert(page_transition_cover_permille(&transition, 1060u) == 500u);
    page_transition_request(&transition, UI_PAGE_DASHBOARD, 1060u);
    assert(page_transition_cover_permille(&transition, 1060u) == 500u);
    page_transition_tick(&transition, 1160u);
    assert(page_transition_phase(&transition) == PAGE_TRANSITION_IDLE);
    assert(page_transition_visible_page(&transition) == UI_PAGE_DASHBOARD);
}

void test_showcase_model(void)
{
    test_idle_rotation();
    test_armed_edge_and_motion_priority();
    test_repeated_armed_does_not_restart_hold();
    test_tick_wrap();
    test_transition_boundaries_and_latest_request();
    test_transition_wrap_and_noop();
    test_transition_latest_cancel_and_continuity();
}
