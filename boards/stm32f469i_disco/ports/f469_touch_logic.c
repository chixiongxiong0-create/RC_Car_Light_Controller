#include "f469_touch_logic.h"

enum { JITTER_PIXELS = 3, SWIPE_PIXELS = 70, SWIPE_MAX_MS = 1200 };

static int32_t distance(int32_t a, int32_t b)
{
    return a > b ? a - b : b - a;
}

F469TouchResult f469_touch_step(F469TouchLogic *touch, bool raw_pressed,
                                int32_t raw_x, int32_t raw_y,
                                uint32_t now_ms, bool allow_swipe_on_start)
{
    F469TouchResult result = {0};
    if (raw_pressed) {
        if (!touch->active) {
            touch->active = true;
            touch->start_x = touch->x = raw_x;
            touch->start_y = touch->y = raw_y;
            touch->start_ms = now_ms;
            touch->swipe_allowed = allow_swipe_on_start;
        } else {
            if (distance(raw_x, touch->x) > JITTER_PIXELS) touch->x = raw_x;
            if (distance(raw_y, touch->y) > JITTER_PIXELS) touch->y = raw_y;
        }
        touch->missing_once = false;
        result.pressed = true;
    } else if (touch->active && !touch->missing_once) {
        touch->missing_once = true;
        result.pressed = true;
    } else if (touch->active) {
        const int32_t dx = touch->x - touch->start_x;
        const int32_t abs_dx = distance(touch->x, touch->start_x);
        const int32_t abs_dy = distance(touch->y, touch->start_y);
        const bool timely = (uint32_t)(now_ms - touch->start_ms) <= SWIPE_MAX_MS;
        result.swiped = touch->swipe_allowed && timely &&
                         (abs_dx >= SWIPE_PIXELS || abs_dy >= SWIPE_PIXELS);
        result.swipe_left = dx < 0;
        if (result.swiped && abs_dx * 2 >= abs_dy * 3)
            result.gesture = dx < 0 ? F469_GESTURE_LEFT : F469_GESTURE_RIGHT;
        else if (result.swiped && abs_dy * 2 >= abs_dx * 3)
            result.gesture = touch->y < touch->start_y ?
                             F469_GESTURE_UP : F469_GESTURE_DOWN;
        else if (timely && abs_dx < SWIPE_PIXELS && abs_dy < SWIPE_PIXELS)
            result.gesture = F469_GESTURE_TAP;
        else result.swiped = false;
        touch->active = false;
        touch->missing_once = false;
    }
    result.x = touch->x;
    result.y = touch->y;
    return result;
}
