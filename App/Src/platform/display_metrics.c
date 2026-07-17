#include "platform/display_metrics.h"

void display_metrics_init(DisplayMetrics *metrics, uint32_t now_ms)
{
    metrics->window_start_ms = now_ms;
    metrics->flush_count = 0u;
    metrics->fps = 0u;
}

void display_metrics_record_flush(DisplayMetrics *metrics)
{
    ++metrics->flush_count;
}

bool display_metrics_update(DisplayMetrics *metrics, uint32_t now_ms)
{
    const uint32_t elapsed = now_ms - metrics->window_start_ms;
    if (elapsed < 1000u) {
        return false;
    }

    uint32_t fps = (metrics->flush_count * 1000u) / elapsed;
    metrics->fps = (uint16_t)((fps > UINT16_MAX) ? UINT16_MAX : fps);
    metrics->flush_count = 0u;
    metrics->window_start_ms = now_ms;
    return true;
}

uint16_t display_metrics_fps(const DisplayMetrics *metrics)
{
    return metrics->fps;
}
