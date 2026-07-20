#include <assert.h>
#include <stdint.h>

#include "input_manager.h"

static VehicleState state_with_aux(float aux)
{
    VehicleState state = {0};
    state.aux_page = aux;
    state.link = LINK_OK;
    return state;
}

static void click(uint32_t now_ms, VehicleState *state)
{
    input_manager_set_button(true, now_ms);
    input_manager_tick(now_ms + 30u, state);
    input_manager_set_button(false, now_ms + 100u);
    input_manager_tick(now_ms + 130u, state);
}

static void test_aux_authority_and_link_fallback(void)
{
    VehicleState state = state_with_aux(0.5f);
    input_manager_init();
    input_manager_tick(0u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    click(10u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);

    state = state_with_aux(0.0f);
    input_manager_init();
    input_manager_tick(2000u, &state);
    click(2010u, &state);
    assert(input_manager_page() == UI_PAGE_FACE);

    state.aux_page = 0.5f;
    input_manager_tick(2200u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    click(2210u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    input_manager_set_touch_available(true);
    input_manager_set_touch_page(UI_PAGE_FACE);
    input_manager_tick(2400u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);

    state.aux_page = -0.5f;
    input_manager_tick(2410u, &state);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);

    state.link = LINK_LOST;
    input_manager_tick(2420u, &state);
    click(2430u, &state);
    assert(input_manager_page() == UI_PAGE_FACE);

    state.link = LINK_OK;
    input_manager_tick(2600u, &state);
    assert(input_manager_page() == UI_PAGE_FACE);
    click(2610u, &state);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    state.aux_page = 0.0f;
    input_manager_tick(2800u, &state);
    assert(input_manager_page() == UI_PAGE_FACE);
}

static void tick_aux(uint32_t now_ms, float aux)
{
    VehicleState state = state_with_aux(aux);
    input_manager_tick(now_ms, &state);
}

static void release_after(uint32_t pressed_at, uint32_t duration_ms, float aux)
{
    input_manager_set_button(true, pressed_at);
    tick_aux(pressed_at + 30u, aux);
    input_manager_set_button(false, pressed_at + duration_ms);
    tick_aux(pressed_at + duration_ms + 30u, aux);
}

static void test_aux_mapping_and_hysteresis(void)
{
    input_manager_init();
    tick_aux(0u, -0.36f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);
    tick_aux(1u, -0.30f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);
    tick_aux(2u, -0.26f);
    assert(input_manager_page() == UI_PAGE_FACE);
    tick_aux(3u, 0.36f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    tick_aux(4u, 0.30f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    tick_aux(5u, 0.26f);
    assert(input_manager_page() == UI_PAGE_FACE);

    input_manager_init();
    tick_aux(10u, -0.35f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);
    tick_aux(11u, 0.35f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    tick_aux(12u, -0.35f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);
}

static void test_button_debounce_and_duration_ranges(void)
{
    input_manager_init();
    tick_aux(0u, 0.0f);

    input_manager_set_button(true, 10u);
    tick_aux(39u, 0.0f);
    input_manager_set_button(false, 39u);
    tick_aux(69u, 0.0f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);

    release_after(100u, 30u, 0.0f);
    assert(input_manager_page() == UI_PAGE_FACE);
    release_after(300u, 800u, 0.0f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);

    release_after(1300u, 801u, 0.0f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    assert(!input_manager_take_brightness_request());
    assert(!input_manager_take_diagnostics_request());
    release_after(2300u, 1999u, 0.0f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    assert(!input_manager_take_brightness_request());

    release_after(6500u, 5000u, 0.0f);
    assert(input_manager_take_diagnostics_request());
    assert(!input_manager_take_diagnostics_request());
    assert(!input_manager_take_brightness_request());
    release_after(4400u, 2000u, 0.0f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
    assert(input_manager_take_brightness_request());
    assert(!input_manager_take_brightness_request());
}

static void test_touch_availability_and_aux_priority(void)
{
    input_manager_init();
    tick_aux(0u, 0.0f);
    input_manager_set_touch_page(UI_PAGE_SHOWCASE);
    tick_aux(1u, 0.0f);
    assert(input_manager_page() == UI_PAGE_DASHBOARD);

    input_manager_set_touch_available(true);
    input_manager_set_touch_page(UI_PAGE_SHOWCASE);
    tick_aux(2u, 0.0f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);

    input_manager_set_touch_page(UI_PAGE_DASHBOARD);
    tick_aux(3u, 0.5f);
    assert(input_manager_page() == UI_PAGE_SHOWCASE);
}

static void test_wrap_safe_debounce_and_press_duration(void)
{
    const uint32_t start = UINT32_MAX - 20u;
    input_manager_init();
    tick_aux(start - 1u, 0.0f);
    input_manager_set_button(true, start);
    tick_aux(start + 30u, 0.0f);
    input_manager_set_button(false, start + 800u);
    tick_aux(start + 830u, 0.0f);
    assert(input_manager_page() == UI_PAGE_FACE);
}

void test_input_manager(void)
{
    test_aux_mapping_and_hysteresis();
    test_button_debounce_and_duration_ranges();
    test_touch_availability_and_aux_priority();
    test_wrap_safe_debounce_and_press_duration();
    test_aux_authority_and_link_fallback();
}
