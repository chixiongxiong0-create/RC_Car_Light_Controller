#pragma once

#include <stddef.h>
#include <string.h>

#include "led/led_controller.h"

enum {
    WS2812_GROUP_COUNT = 4,
    WS2812_GROUP_MAX_PIXELS = 8,
    WS2812_TOTAL_PIXELS = 24
};

static const size_t WS2812_GROUP_LENGTHS[WS2812_GROUP_COUNT] = {
    4u, 4u, 8u, 8u
};

typedef struct {
    LedRgb groups[WS2812_GROUP_COUNT][WS2812_GROUP_MAX_PIXELS];
} Ws2812Frame;

static inline void ws2812_frame_clear(Ws2812Frame *frame)
{
    if (frame != NULL) {
        memset(frame, 0, sizeof *frame);
    }
}
