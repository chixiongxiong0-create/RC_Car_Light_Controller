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

    two_screen_face_swipe(&ui, true, 1000u);
    assert(two_screen_view(&ui).manual);
    assert(two_screen_view(&ui).mood == FACE_HAPPY);
    two_screen_tick(&ui, 10999u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_HAPPY);
    two_screen_tick(&ui, 11000u, &car, false);
    assert(!two_screen_view(&ui).manual);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_face_swipe(&ui, true, 12000u);
    car.link = LINK_LOST;
    two_screen_tick(&ui, 12001u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_LINK_LOST);
    car.link = LINK_OK;
    two_screen_tick(&ui, 12002u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_HAPPY);
    two_screen_tick(&ui, 22000u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_select_main(&ui, MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_OVERVIEW);
    two_screen_page_swipe(&ui, true);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);
    two_screen_page_swipe(&ui, true);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);
    two_screen_select_main(&ui, MAIN_FACE);
    two_screen_select_main(&ui, MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);

    two_screen_button(&ui, true);
    assert(two_screen_view(&ui).main == MAIN_INFO);
    two_screen_button(&ui, false);
    assert(two_screen_view(&ui).main == MAIN_FACE);

    two_screen_init(&ui);
    two_screen_face_swipe(&ui, true, UINT32_MAX - 100u);
    two_screen_tick(&ui, 9898u, &car, false);
    assert(two_screen_view(&ui).manual);
    two_screen_tick(&ui, 9899u, &car, false);
    assert(!two_screen_view(&ui).manual);

    two_screen_init(&ui);
    two_screen_page_swipe(&ui, true);
    assert(two_screen_view(&ui).main == MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_OVERVIEW);
    two_screen_page_swipe(&ui, true);
    assert(two_screen_view(&ui).info == INFO_DIAGNOSTICS);
    two_screen_page_swipe(&ui, false);
    assert(two_screen_view(&ui).main == MAIN_INFO);
    assert(two_screen_view(&ui).info == INFO_OVERVIEW);
    two_screen_page_swipe(&ui, false);
    assert(two_screen_view(&ui).main == MAIN_FACE);

    two_screen_face_swipe(&ui, true, 20000u);
    assert(two_screen_view(&ui).mood == FACE_HAPPY);
    two_screen_face_swipe(&ui, true, 20001u);
    assert(two_screen_view(&ui).mood == FACE_SLEEPY);
    two_screen_face_tap(&ui);
    assert(!two_screen_view(&ui).manual);

    car.link = LINK_OK;
    car.throttle = 0.0f;
    car.steering = 0.8f;
    two_screen_tick(&ui, 21000u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_TURN);
    car.steering = 0.0f;
    car.throttle = 0.9f;
    two_screen_tick(&ui, 21001u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_RAGE);

    car.throttle = 0.0f;
    two_screen_face_swipe(&ui, true, 21050u);
    two_screen_tick(&ui, 21100u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_BRAKE);
    two_screen_face_tap(&ui);
    two_screen_tick(&ui, 22101u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    car.link = LINK_LOST;
    two_screen_tick(&ui, 22200u, &car, false);
    car.link = LINK_OK;
    two_screen_tick(&ui, 22201u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_HAPPY);
    two_screen_tick(&ui, 23502u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_IDLE);

    two_screen_face_swipe(&ui, true, 24000u);
    two_screen_tick(&ui, 29001u, &car, false);
    assert(two_screen_view(&ui).action != FACE_ACTION_NONE);
    car.link = LINK_LOST;
    two_screen_tick(&ui, 29002u, &car, false);
    assert(two_screen_view(&ui).mood == FACE_LINK_LOST);
    assert(two_screen_view(&ui).action == FACE_ACTION_NONE);
    return 0;
}
