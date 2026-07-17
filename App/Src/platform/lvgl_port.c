#include "platform/lvgl_port.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ltdc.h"
#include "lvgl.h"
#include "platform/display_metrics.h"
#include "platform/lvgl_port_math.h"

__attribute__((section(".ltdc_scanout"), aligned(32)))
static uint16_t scanout[PHYSICAL_WIDTH * PHYSICAL_HEIGHT];

/* Deliberately a single scanout buffer. LTDC can scan while LVGL writes, so
 * moving-content hardware acceptance is blocking because tearing is possible. */

__attribute__((section(".lvgl_draw"), aligned(32)))
static uint16_t draw_buf[UI_WIDTH * DRAW_LINES];

static lv_display_t *display;
static uint32_t last_tick_ms;
static bool tick_started;
static DisplayMetrics metrics;

static void clean_scanout_cache_for_rotated_area(const lv_area_t *area)
{
    /* Each logical x maps to a separate physical scanline. Clean every row;
       the rotated rectangle is not contiguous in physical memory. */
    for (int32_t x = area->x1; x <= area->x2; ++x) {
        LvglCacheRange range = lvgl_port_cache_range_for_column(
            (uintptr_t)scanout,
            (uint16_t)x,
            (uint16_t)area->y1,
            (uint16_t)area->y2);
        SCB_CleanDCache_by_Addr((uint32_t *)range.address, (int32_t)range.size);
    }
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    const uint16_t *src = (const uint16_t *)px_map;

    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            scanout[lvgl_port_scanout_index((uint16_t)x, (uint16_t)y)] = *src++;
        }
    }

    clean_scanout_cache_for_rotated_area(area);
    display_metrics_record_flush(&metrics);
    lv_display_flush_ready(disp);
}

bool lvgl_port_init(void)
{
    memset(scanout, 0, sizeof(scanout));
    SCB_CleanDCache_by_Addr((uint32_t *)scanout, (int32_t)sizeof(scanout));

    lv_init();
    display = lv_display_create(UI_WIDTH, UI_HEIGHT);
    if (display == NULL) {
        return false;
    }
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_cb);
    if (HAL_LTDC_SetAddress(&hltdc, (uint32_t)scanout, 0u) != HAL_OK) {
        lv_display_delete(display);
        display = NULL;
        return false;
    }

    last_tick_ms = HAL_GetTick();
    display_metrics_init(&metrics, last_tick_ms);
    tick_started = true;
    return true;
}

void lvgl_port_tick(uint32_t now_ms)
{
    if (!tick_started) {
        last_tick_ms = now_ms;
        tick_started = true;
    }

    lv_tick_inc(lvgl_port_tick_delta(now_ms, last_tick_ms));
    last_tick_ms = now_ms;
    (void)lv_timer_handler();
    (void)display_metrics_update(&metrics, now_ms);
}

uint16_t lvgl_port_fps(void)
{
    return display_metrics_fps(&metrics);
}
