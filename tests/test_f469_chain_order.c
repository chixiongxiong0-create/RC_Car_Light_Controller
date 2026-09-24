#include <assert.h>
#include <stdio.h>
#include "ws2812_f469.h"

int main(void)
{
    Ws2812Frame frame = {0};
    unsigned serial = 1u;
    for (unsigned group = 0; group < WS2812_GROUP_COUNT; ++group)
        for (unsigned i = 0; i < WS2812_GROUP_LENGTHS[group]; ++i)
            frame.groups[group][i].r = (uint8_t)serial++;
    for (unsigned i = 0; i < 8u; ++i)
        assert(f469_ws2812_chain_pixel(&frame, 0u, i).r == i + 1u);
    for (unsigned i = 0; i < 16u; ++i)
        assert(f469_ws2812_chain_pixel(&frame, 1u, i).r == i + 9u);
    puts("F469 chain order passed");
    return 0;
}
