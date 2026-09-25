#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool active;
    bool missing_once;
    bool swipe_allowed;
    int32_t x;
    int32_t y;
    int32_t start_x;
    int32_t start_y;
    uint32_t start_ms;
} F469TouchLogic;

typedef struct {
    bool pressed;
    bool swiped;
    bool swipe_left;
    int32_t x;
    int32_t y;
} F469TouchResult;

F469TouchResult f469_touch_step(F469TouchLogic *touch, bool raw_pressed,
                                int32_t raw_x, int32_t raw_y,
                                uint32_t now_ms, bool allow_swipe_on_start);
