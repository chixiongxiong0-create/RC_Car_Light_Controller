#include "lighting/lighting_service.h"

#include <string.h>

void lighting_service_init(LightingService *service)
{
    if (service == NULL) {
        return;
    }
    memset(service, 0, sizeof *service);
    lighting_controller_init(&service->controller);
}

uint32_t lighting_service_tick(LightingService *service,
                               uint32_t now_ms,
                               const VehicleState *real_state,
                               bool low_battery,
                               bool board_fault,
                               LightingApplyFn apply,
                               LightingSubmitFn submit,
                               void *ctx)
{
    if (service == NULL) {
        return 0u;
    }
    lighting_controller_render(&service->controller, now_ms, real_state,
                               low_battery, board_fault, &service->frame);
    if (apply != NULL) {
        apply(service->frame.front_duty,
              service->frame.roof_spot_duty, ctx);
    }
    if (submit != NULL) {
        (void)submit(now_ms, &service->frame.ws2812, ctx);
    }
    return service->frame.estimated_ma;
}
