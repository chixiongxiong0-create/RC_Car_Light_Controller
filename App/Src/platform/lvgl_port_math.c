#include "platform/lvgl_port_math.h"

LvglPhysicalPoint lvgl_port_map_clockwise(uint16_t logical_x,
                                          uint16_t logical_y)
{
    LvglPhysicalPoint point = {
        .x = logical_y,
        .y = (uint16_t)((LVGL_PORT_PHYSICAL_HEIGHT - 1u) - logical_x),
    };
    return point;
}

size_t lvgl_port_scanout_index(uint16_t logical_x, uint16_t logical_y)
{
    LvglPhysicalPoint point = lvgl_port_map_clockwise(logical_x, logical_y);
    return ((size_t)point.y * LVGL_PORT_PHYSICAL_WIDTH) + point.x;
}

LvglCacheRange lvgl_port_cache_range_for_column(uintptr_t scanout_address,
                                                uint16_t logical_x,
                                                uint16_t logical_y1,
                                                uint16_t logical_y2)
{
    const uintptr_t first = scanout_address +
        (lvgl_port_scanout_index(logical_x, logical_y1) * sizeof(uint16_t));
    const uintptr_t end = scanout_address +
        ((lvgl_port_scanout_index(logical_x, logical_y2) + 1u) * sizeof(uint16_t));
    const uintptr_t aligned_first = first & ~(uintptr_t)(LVGL_PORT_CACHE_LINE_SIZE - 1u);
    const uintptr_t aligned_end = (end + LVGL_PORT_CACHE_LINE_SIZE - 1u) &
        ~(uintptr_t)(LVGL_PORT_CACHE_LINE_SIZE - 1u);
    LvglCacheRange range = {
        .address = aligned_first,
        .size = (size_t)(aligned_end - aligned_first),
    };
    return range;
}

uint32_t lvgl_port_tick_delta(uint32_t now_ms, uint32_t previous_ms)
{
    return now_ms - previous_ms;
}
