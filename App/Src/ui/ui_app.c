#include "ui/ui_app.h"

#include "ui/screen_dashboard.h"

static lv_obj_t *dashboard;

void ui_app_init(void)
{
    dashboard = screen_dashboard_create();
    lv_screen_load(dashboard);
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
    (void)now_ms;
    if (dashboard != NULL && state != NULL) {
        screen_dashboard_update(state);
    }
}
