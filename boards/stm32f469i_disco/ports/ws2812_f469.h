#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "led/ws2812_frame.h"
static inline LedRgb f469_ws2812_chain_pixel(const Ws2812Frame *frame,
                                               unsigned chain, unsigned index)
{
    const unsigned first_group = chain == 0u ? 0u : 2u;
    const unsigned first_length = (unsigned)WS2812_GROUP_LENGTHS[first_group];
    const unsigned group = first_group + (index >= first_length);
    const unsigned offset = index < first_length ? index : index - first_length;
    return frame->groups[group][offset];
}
void f469_ws2812_force_low(void);
bool f469_ws2812_init(void);
bool f469_ws2812_submit(uint32_t now_ms, const Ws2812Frame *frame);
uint32_t f469_ws2812_busy_drops(void);
uint32_t f469_ws2812_errors(void);
