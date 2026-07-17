#include <assert.h>

#include "platform/display_metrics.h"

void test_display_metrics(void)
{
    DisplayMetrics metrics;
    display_metrics_init(&metrics, 100u);
    display_metrics_record_flush(&metrics);
    display_metrics_record_flush(&metrics);
    assert(!display_metrics_update(&metrics, 1099u));
    assert(display_metrics_fps(&metrics) == 0u);
    assert(display_metrics_update(&metrics, 1100u));
    assert(display_metrics_fps(&metrics) == 2u);

    display_metrics_init(&metrics, 0xfffffff0u);
    display_metrics_record_flush(&metrics);
    assert(display_metrics_update(&metrics, 0x000003d8u));
    assert(display_metrics_fps(&metrics) == 1u);
}
