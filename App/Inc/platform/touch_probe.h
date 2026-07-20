#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef bool (*TouchProbeRead)(uint8_t address, uint16_t reg,
                               bool reg16, uint8_t *data,
                               uint16_t length, uint32_t timeout_ms,
                               void *ctx);

typedef enum {
    TOUCH_NONE,
    TOUCH_FT_FAMILY,
    TOUCH_GT_FAMILY
} TouchController;

TouchController touch_probe_identify(TouchProbeRead read, void *ctx);

#ifndef TOUCH_PROBE_HOST_TEST
TouchController touch_probe_boot(void);
#endif
