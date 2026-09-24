#include <assert.h>
#include <stdint.h>
#include "ui/two_screen_controller.h"

int main(void)
{
    TwoScreenController ui;
    VehicleState car = {0};
    car.link = LINK_OK;
    two_screen_init(&ui);
    assert(two_screen_view(&ui).main == MAIN_FACE);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_swipe(&ui, true, 1000u);
    assert(two_screen_view(&ui).manual);
    assert(two_screen_view(&ui).mood == FACE_FOCUSED);
    two_screen_tick(&ui, 10999u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_FOCUSED);
    two_screen_tick(&ui, 11000u, &car, false);
    assert(!two_screen_view(&ui).manual);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_swipe(&ui, true, 12000u);
    car.link = LINK_LOST;
    two_screen_tick(&ui, 12001u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_LINK_LOST);
    car.link = LINK_OK;
    two_screen_tick(&ui, 12002u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_FOCUSED);
    two_screen_tick(&ui, 22000u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_select_main(&ui, MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_OVERVIEW);
    two_screen_swipe(&ui, true, 23000u);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);
    two_screen_swipe(&ui, true, 23001u);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);
    two_screen_select_main(&ui, MAIN_FACE);
    two_screen_select_main(&ui, MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);

    two_screen_button(&ui, true);
    assert(two_screen_view(&ui).main == MAIN_INFO);
    two_screen_button(&ui, false);
    assert(two_screen_view(&ui).main == MAIN_FACE);

    two_screen_init(&ui);
    two_screen_swipe(&ui, true, UINT32_MAX - 100u);
    two_screen_tick(&ui, 9898u, &car, false);
    assert(two_screen_view(&ui).manual);
    two_screen_tick(&ui, 9899u, &car, false);
    assert(!two_screen_view(&ui).manual);
    return 0;
}
