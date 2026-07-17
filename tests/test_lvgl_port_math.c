#include <assert.h>
#include <stdint.h>

#include "platform/lvgl_port_math.h"

void test_lvgl_port_math(void)
{
    LvglPhysicalPoint point = lvgl_port_map_clockwise(0u, 0u);
    assert(point.x == 0u);
    assert(point.y == 319u);

    point = lvgl_port_map_clockwise(319u, 239u);
    assert(point.x == 239u);
    assert(point.y == 0u);

    assert(lvgl_port_scanout_index(12u, 34u) == ((319u - 12u) * 240u + 34u));

    LvglCacheRange range = lvgl_port_cache_range_for_column(
        (uintptr_t)0x90000000u, 10u, 3u, 18u);
    assert(range.address == (uintptr_t)0x90024360u);
    assert(range.size == 64u);

    LvglCacheRange next = lvgl_port_cache_range_for_column(
        (uintptr_t)0x90000000u, 11u, 3u, 18u);
    assert(next.address != range.address);
    assert(next.address < range.address);

    assert(lvgl_port_tick_delta(5u, 0xfffffff0u) == 21u);
}
