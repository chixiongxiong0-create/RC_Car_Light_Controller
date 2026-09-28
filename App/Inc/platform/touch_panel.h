#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform/touch_probe.h"

typedef struct {
    bool pressed;
    uint16_t x;
    uint16_t y;
} TouchSample;

typedef enum {
    TOUCH_POLL_NO_DATA = 0,
    TOUCH_POLL_UPDATE = 1,
    TOUCH_POLL_IO_ERROR = 2,
} TouchPollResult;

/* False means no new controller sample; keep the previous pointer state. */
bool touch_panel_decode_ft(uint8_t status, const uint8_t point[4],
                           TouchSample *sample);
bool touch_panel_decode_gt(uint8_t status, const uint8_t point[8],
                           TouchSample *sample);
bool touch_panel_map_to_landscape(TouchSample *sample);

typedef bool (*TouchPanelWrite)(uint8_t address, uint16_t reg, bool reg16,
                                const uint8_t *data, uint16_t length,
                                uint32_t timeout_ms, void *ctx);

TouchPollResult touch_panel_poll(TouchProbeResult controller, TouchProbeRead read,
                                 TouchPanelWrite write, void *ctx,
                                 TouchSample *sample);

#ifndef TOUCH_PROBE_HOST_TEST
TouchPollResult touch_panel_poll_hal(TouchProbeResult controller,
                                     TouchSample *sample);
#endif
