#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lighting/lighting_controller.h"

typedef void (*LightingApplyFn)(uint16_t front_duty,
                                uint16_t roof_spot_duty,
                                void *ctx);
typedef bool (*LightingSubmitFn)(uint32_t now_ms,
                                 const Ws2812Frame *frame,
                                 void *ctx);

typedef struct {
    LightingController controller;
    LightingFrame frame;
} LightingService;

void lighting_service_init(LightingService *service);
uint32_t lighting_service_tick(LightingService *service,
                               uint32_t now_ms,
                               const VehicleState *real_state,
                               bool low_battery,
                               bool board_fault,
                               LightingApplyFn apply,
                               LightingSubmitFn submit,
                               void *ctx);
