#include "ui/ui_app.h"

#include "app_config.h"
#include "diagnostics.h"
#include "input_manager.h"
#include "ui/screen_dashboard.h"
#include "ui/screen_face.h"
#include "ui/screen_showcase.h"
#include "ui/ui_theme.h"

static lv_obj_t *dashboard;
static lv_obj_t *face;
static lv_obj_t *showcase;
static lv_obj_t *shutter;
static lv_obj_t *shutter_edge;
static PageTransition transition;
static lv_obj_t *diagnostics_panel;
static lv_obj_t *diagnostics_label;
static bool diagnostics_visible;
static uint32_t diagnostics_updated_ms;

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
    page_transition_init(&transition, input_manager_page(), 0u);
    lv_screen_load(page_object(page_transition_visible_page(&transition)));

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

    diagnostics_panel = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(diagnostics_panel, 8, 8);
    lv_obj_set_size(diagnostics_panel, 304, 224);
    lv_obj_set_style_bg_color(diagnostics_panel, lv_color_hex(0x080B0E), 0);
    lv_obj_set_style_bg_opa(diagnostics_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_color(diagnostics_panel, UI_COLOR_YELLOW, 0);
    lv_obj_set_style_border_width(diagnostics_panel, 2, 0);
    lv_obj_remove_flag(diagnostics_panel, LV_OBJ_FLAG_SCROLLABLE);
    diagnostics_label = lv_label_create(diagnostics_panel);
    lv_obj_set_style_text_color(diagnostics_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(diagnostics_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(diagnostics_label, "DIAGNOSTICS");
    lv_obj_add_flag(diagnostics_panel, LV_OBJ_FLAG_HIDDEN);
    diagnostics_visible = false;
    diagnostics_updated_ms = 0u;
}

static void update_diagnostics(uint32_t now_ms)
{
    char text[256];
    const DiagnosticsSnapshot *d = diagnostics_get();
    if (input_manager_take_diagnostics_request()) {
        diagnostics_visible = !diagnostics_visible;
        if (diagnostics_visible) {
            lv_obj_remove_flag(diagnostics_panel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(diagnostics_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (!diagnostics_visible || (uint32_t)(now_ms - diagnostics_updated_ms) < 250u) {
        return;
    }
    const char *health = d->state == HEALTH_FAULT ? "FAULT" :
                         d->state == HEALTH_DEGRADED ? "DEGRADED" :
                         d->state == HEALTH_OK ? "OK" : "BOOTING";
    (void)lv_snprintf(text, sizeof text,
                      "DIAGNOSTICS  %s\n"
                      "FPS %u   MSP AGE %lu ms\n"
                      "MSP TIMEOUTS %lu\nUART OVERRUNS %lu\n"
                      "FRAME MISSES %lu\nMAX LOOP %lu us\n"
                      "LED %lu mA\nRESET 0x%08lX\nTOUCH %s",
                      health, (unsigned)d->fps, (unsigned long)d->msp_age_ms,
                      (unsigned long)d->msp_timeouts,
                      (unsigned long)d->uart_overruns,
                      (unsigned long)d->frame_misses,
                      (unsigned long)d->max_loop_us,
                      (unsigned long)d->led_current_ma,
                      (unsigned long)d->reset_flags,
                      d->touch_available ? "YES" : "NO");
    lv_label_set_text(diagnostics_label, text);
    lv_obj_set_style_border_color(diagnostics_panel,
        d->state == HEALTH_FAULT ? lv_color_hex(0xFF2020) : UI_COLOR_YELLOW, 0);
    diagnostics_updated_ms = now_ms;
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state, bool low_battery)
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
    update_diagnostics(now_ms);

    if (visible == UI_PAGE_DASHBOARD) {
        screen_dashboard_update(state);
    } else if (visible == UI_PAGE_FACE) {
        screen_face_update(now_ms, state, low_battery);
    } else {
        screen_showcase_update(now_ms, state);
    }
}
