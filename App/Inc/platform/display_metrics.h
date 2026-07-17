#ifndef DISPLAY_METRICS_H
#define DISPLAY_METRICS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t window_start_ms;
    uint32_t flush_count;
    uint16_t fps;
} DisplayMetrics;

void display_metrics_init(DisplayMetrics *metrics, uint32_t now_ms);
void display_metrics_record_flush(DisplayMetrics *metrics, bool is_last);
bool display_metrics_update(DisplayMetrics *metrics, uint32_t now_ms);
uint16_t display_metrics_fps(const DisplayMetrics *metrics);
uint16_t display_metrics_clamp_ui_fps(uint16_t fps);

#endif
