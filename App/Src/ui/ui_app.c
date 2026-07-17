#include "ui/ui_app.h"

#include "ui/screen_dashboard.h"
#include "ui/screen_face.h"

#define UI_LOW_BATTERY_THRESHOLD_V 10.5f

static lv_obj_t *dashboard;
static lv_obj_t *face;

void ui_app_init(void)
{
    dashboard = screen_dashboard_create();
    face = screen_face_create();
    lv_obj_add_flag(face, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(dashboard);
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
    if (dashboard != NULL && state != NULL) {
        screen_dashboard_update(state);
    }
    if (face != NULL && state != NULL) {
        const bool low_battery = state->battery_v > 0.0f &&
                                 state->battery_v <= UI_LOW_BATTERY_THRESHOLD_V;
        screen_face_update(now_ms, state, low_battery);
    }
}
