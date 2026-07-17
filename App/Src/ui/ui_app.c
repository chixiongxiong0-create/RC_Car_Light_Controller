#include "ui/ui_app.h"

#include "app_config.h"
#include "ui/screen_dashboard.h"
#include "ui/screen_face.h"
#include "ui/low_battery_policy.h"

static lv_obj_t *dashboard;
static lv_obj_t *face;
static LowBatteryPolicy battery_policy;

void ui_app_init(void)
{
    dashboard = screen_dashboard_create();
    face = screen_face_create();
    low_battery_policy_init(&battery_policy, APP_BATTERY_CELL_COUNT);
    lv_obj_add_flag(face, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(dashboard);
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
    if (dashboard != NULL && state != NULL) {
        screen_dashboard_update(state);
    }
    if (face != NULL && state != NULL) {
        const bool low_battery = low_battery_policy_update(
            &battery_policy, state->battery_v, state->battery_v > 0.0f);
        screen_face_update(now_ms, state, low_battery);
    }
}
