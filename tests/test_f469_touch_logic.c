#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "f469_touch_logic.h"

int main(void)
{
    F469TouchLogic touch = {0};
    F469TouchResult r = f469_touch_step(&touch, true, 100, 100, 0, true);
    assert(r.pressed && r.x == 100 && r.y == 100);

    r = f469_touch_step(&touch, true, 102, 101, 20, true);
    assert(r.pressed && r.x == 100 && r.y == 100);
    r = f469_touch_step(&touch, true, 106, 105, 40, true);
    assert(r.pressed && r.x == 106 && r.y == 105);
    r = f469_touch_step(&touch, false, 0, 0, 60, true);
    assert(r.pressed && !r.swiped);
    r = f469_touch_step(&touch, false, 0, 0, 80, true);
    assert(!r.pressed && !r.swiped);

    r = f469_touch_step(&touch, true, 102, 102, 100, true);
    assert(r.pressed && r.x == 102 && r.y == 102);
    f469_touch_step(&touch, true, 103, 102, 120, true);
    f469_touch_step(&touch, false, 0, 0, 140, true);
    r = f469_touch_step(&touch, false, 0, 0, 160, true);
    assert(!r.swiped);

    f469_touch_step(&touch, true, 300, 200, 200, true);
    f469_touch_step(&touch, true, 200, 215, 300, true);
    f469_touch_step(&touch, false, 0, 0, 320, true);
    r = f469_touch_step(&touch, false, 0, 0, 340, true);
    assert(r.swiped && r.swipe_left);

    f469_touch_step(&touch, true, 200, 200, 350, true);
    f469_touch_step(&touch, true, 295, 210, 370, true);
    f469_touch_step(&touch, false, 0, 0, 390, true);
    r = f469_touch_step(&touch, false, 0, 0, 410, true);
    assert(r.swiped && !r.swipe_left);

    f469_touch_step(&touch, true, 300, 200, 450, true);
    f469_touch_step(&touch, true, 200, 300, 550, true);
    f469_touch_step(&touch, false, 0, 0, 570, true);
    r = f469_touch_step(&touch, false, 0, 0, 590, true);
    assert(!r.swiped);

    f469_touch_step(&touch, true, 300, 440, 650, false);
    f469_touch_step(&touch, true, 200, 440, 750, true);
    f469_touch_step(&touch, false, 0, 0, 770, true);
    r = f469_touch_step(&touch, false, 0, 0, 790, true);
    assert(!r.swiped);

    f469_touch_step(&touch, true, 300, 200, 850, true);
    f469_touch_step(&touch, true, 200, 200, 2150, true);
    f469_touch_step(&touch, false, 0, 0, 2170, true);
    r = f469_touch_step(&touch, false, 0, 0, 2190, true);
    assert(!r.swiped);
    return 0;
}
