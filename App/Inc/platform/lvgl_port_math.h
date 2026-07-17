#ifndef LVGL_PORT_MATH_H
#define LVGL_PORT_MATH_H

#include <stddef.h>
#include <stdint.h>

#define LVGL_PORT_PHYSICAL_WIDTH 240u
#define LVGL_PORT_PHYSICAL_HEIGHT 320u
#define LVGL_PORT_CACHE_LINE_SIZE 32u

typedef struct {
    uint16_t x;
    uint16_t y;
} LvglPhysicalPoint;

typedef struct {
    uintptr_t address;
    size_t size;
} LvglCacheRange;

LvglPhysicalPoint lvgl_port_map_clockwise(uint16_t logical_x,
                                          uint16_t logical_y);
size_t lvgl_port_scanout_index(uint16_t logical_x, uint16_t logical_y);
LvglCacheRange lvgl_port_cache_range_for_column(uintptr_t scanout_address,
                                                uint16_t logical_x,
                                                uint16_t logical_y1,
                                                uint16_t logical_y2);
uint32_t lvgl_port_tick_delta(uint32_t now_ms, uint32_t previous_ms);

#endif
