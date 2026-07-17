#include <assert.h>

#include "platform/display_metrics.h"

void test_display_metrics(void)
{
    DisplayMetrics metrics;
    display_metrics_init(&metrics, 100u);
    for (unsigned frame = 0u; frame < 28u; ++frame) {
        for (unsigned tile = 0u; tile < 10u; ++tile) {
            display_metrics_record_flush(&metrics, tile == 9u);
        }
    }
    assert(display_metrics_update(&metrics, 1100u));
    assert(display_metrics_fps(&metrics) == 28u);

    display_metrics_init(&metrics, 0xfffffff0u);
    display_metrics_record_flush(&metrics, false);
    display_metrics_record_flush(&metrics, true);
    assert(display_metrics_update(&metrics, 0x000003d8u));
    assert(display_metrics_fps(&metrics) == 1u);

    assert(display_metrics_clamp_ui_fps(0u) == 0u);
    assert(display_metrics_clamp_ui_fps(28u) == 28u);
    assert(display_metrics_clamp_ui_fps(99u) == 99u);
    assert(display_metrics_clamp_ui_fps(100u) == 99u);
    assert(display_metrics_clamp_ui_fps(UINT16_MAX) == 99u);
}
