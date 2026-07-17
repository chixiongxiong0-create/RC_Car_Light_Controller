#include "ui/ui_app.h"

#include "app_config.h"
#include "input_manager.h"
#include "ui/screen_dashboard.h"
#include "ui/screen_face.h"
#include "ui/low_battery_policy.h"
#include "ui/screen_showcase.h"
#include "ui/ui_theme.h"

static lv_obj_t *dashboard;
static lv_obj_t *face;
static lv_obj_t *showcase;
static lv_obj_t *shutter;
static lv_obj_t *shutter_edge;
static LowBatteryPolicy battery_policy;
static PageTransition transition;

static lv_obj_t *page_object(UiPage page)
{
    if (page == UI_PAGE_FACE) {
        return face;
    }
    if (page == UI_PAGE_SHOWCASE) {
        return showcase;
    }
    return dashboard;
}

static void update_shutter(uint32_t now_ms)
{
    const uint16_t cover = page_transition_cover_permille(&transition, now_ms);
    const int32_t width = (320 * (int32_t)cover) / 1000;
    if (cover == 0u) {
        lv_obj_add_flag(shutter, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(shutter_edge, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    lv_obj_remove_flag(shutter, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(shutter_edge, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(shutter, 320 - width, 0);
    lv_obj_set_size(shutter, width, 240);
    lv_obj_set_pos(shutter_edge, 304 - width, 0);
}

void ui_app_init(void)
{
    dashboard = screen_dashboard_create();
    face = screen_face_create();
    showcase = screen_showcase_create();
    low_battery_policy_init(&battery_policy, APP_BATTERY_CELL_COUNT);
    page_transition_init(&transition, input_manager_page(), 0u);
    lv_screen_load(dashboard);

    shutter = lv_obj_create(lv_layer_top());
    lv_obj_remove_flag(shutter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(shutter, UI_COLOR_YELLOW, 0);
    lv_obj_set_style_bg_opa(shutter, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shutter, 0, 0);
    lv_obj_set_style_radius(shutter, 0, 0);
    shutter_edge = lv_obj_create(lv_layer_top());
    lv_obj_remove_flag(shutter_edge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(shutter_edge, 16, 240);
    lv_obj_set_style_bg_color(shutter_edge, lv_color_hex(0x080B0E), 0);
    lv_obj_set_style_bg_opa(shutter_edge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shutter_edge, 0, 0);
    lv_obj_set_style_radius(shutter_edge, 0, 0);
    lv_obj_add_flag(shutter, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(shutter_edge, LV_OBJ_FLAG_HIDDEN);
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
    if (dashboard == NULL || state == NULL) {
        return;
    }
    page_transition_request(&transition, input_manager_page(), now_ms);
    const UiPage before = page_transition_visible_page(&transition);
    page_transition_tick(&transition, now_ms);
    const UiPage visible = page_transition_visible_page(&transition);
    if (visible != before) {
        lv_screen_load(page_object(visible));
    }
    update_shutter(now_ms);

    if (visible == UI_PAGE_DASHBOARD) {
        screen_dashboard_update(state);
    } else if (visible == UI_PAGE_FACE) {
        const bool low_battery = low_battery_policy_update(
            &battery_policy, state->battery_v, state->battery_v > 0.0f);
        screen_face_update(now_ms, state, low_battery);
    } else {
        screen_showcase_update(now_ms, state);
    }
}
