#include "ui/f469_ui.h"

#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "ui/f469_face.h"
#include "ui/f469_info.h"
#include "ui/two_screen_controller.h"
#include "ui/ui_theme.h"

static TwoScreenController navigation;
static lv_obj_t *face_area;
static lv_obj_t *info_area;
static lv_obj_t *face_tab;
static lv_obj_t *info_tab;
static lv_obj_t *link_label;
static lv_obj_t *battery_label;
static lv_obj_t *demo_label;
static int previous_main = -1;
static int previous_demo = -1;

static void set_text_if_changed(lv_obj_t *obj, const char *text)
{
    if (strcmp(lv_label_get_text(obj), text) != 0) lv_label_set_text(obj, text);
}

static void on_tab(lv_event_t *event)
{
    const MainScreen main = (MainScreen)(uintptr_t)lv_event_get_user_data(event);
    two_screen_select_main(&navigation, main);
}

static lv_obj_t *make_tab(lv_obj_t *bar, const char *text, int32_t x,
                           MainScreen main)
{
    lv_obj_t *button = lv_button_create(bar);
    lv_obj_set_pos(button, x, 6);
    lv_obj_set_size(button, 145, 52);
    lv_obj_set_ext_click_area(button, 4);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_add_event_cb(button, on_tab, LV_EVENT_CLICKED, (void *)(uintptr_t)main);
    lv_obj_t *caption = lv_label_create(button);
    lv_label_set_text(caption, text);
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_24, 0);
    lv_obj_center(caption);
    return button;
}

bool f469_ui_init(void)
{
    if (lv_display_get_default() == NULL) return false;
    two_screen_init(&navigation);
    ui_theme_init();
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 800, 480);
    lv_obj_set_style_bg_color(screen, UI_COLOR_BG, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_set_style_border_width(screen, 0, 0);

    lv_obj_t *bar = lv_obj_create(screen);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, 800, 64);
    ui_theme_apply_top_status(bar);
    face_tab = make_tab(bar, "FACE", 8, MAIN_FACE);
    info_tab = make_tab(bar, "INFO", 161, MAIN_INFO);
    link_label = lv_label_create(bar);
    lv_obj_set_pos(link_label, 435, 20);
    lv_obj_set_style_text_font(link_label, &lv_font_montserrat_16, 0);
    battery_label = lv_label_create(bar);
    lv_obj_set_pos(battery_label, 645, 20);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_16, 0);
    demo_label = lv_label_create(bar);
    lv_label_set_text(demo_label, "DEMO");
    lv_obj_set_pos(demo_label, 344, 20);
    lv_obj_set_style_text_color(demo_label, lv_color_hex(0xF1A7FF), 0);
    lv_obj_set_style_text_font(demo_label, &lv_font_montserrat_16, 0);
    lv_obj_add_flag(demo_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, 64);
    lv_obj_set_size(content, 800, 416);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    face_area = f469_face_create(content);
    info_area = f469_info_create(content);
    lv_obj_add_flag(info_area, LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen);
    return true;
}

void f469_ui_button(bool touch_available)
{
    two_screen_button(&navigation, touch_available);
}

void f469_ui_swipe(bool left, uint32_t now_ms)
{
    two_screen_swipe(&navigation, left, now_ms);
}

bool f469_ui_brightness_hit_test(int32_t x, int32_t y)
{
    const TwoScreenView view = two_screen_view(&navigation);
    return view.main == MAIN_INFO && view.info == INFO_OVERVIEW &&
           f469_info_brightness_hit_test(x, y);
}

void f469_ui_tick(uint32_t now_ms, const VehicleState *state,
                  const DiagnosticsSnapshot *diagnostics,
                  bool low_battery, bool demo)
{
    if (face_area == NULL) return;
    two_screen_tick(&navigation, now_ms, state, low_battery);
    const TwoScreenView view = two_screen_view(&navigation);
    if ((int)view.main != previous_main) {
        if (view.main == MAIN_FACE) {
            lv_obj_remove_flag(face_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(info_area, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(face_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(info_area, LV_OBJ_FLAG_HIDDEN);
        }
        lv_obj_set_style_bg_color(face_tab, view.main == MAIN_FACE ?
                                  UI_COLOR_YELLOW : UI_COLOR_PANEL, 0);
        lv_obj_set_style_bg_color(info_tab, view.main == MAIN_INFO ?
                                  UI_COLOR_YELLOW : UI_COLOR_PANEL, 0);
        previous_main = view.main;
    }
    set_text_if_changed(link_label, state == NULL ? "START" :
        state->link == LINK_OK ? "LINK OK" :
        state->link == LINK_STALE ? "STALE" :
        state->link == LINK_LOST ? "LINK LOST" : "START");
    char battery_text[16];
    if (state != NULL && state->battery_valid && state->link == LINK_OK) {
        (void)snprintf(battery_text, sizeof battery_text, "%.1f V", (double)state->battery_v);
    } else {
        (void)snprintf(battery_text, sizeof battery_text, "--.- V");
    }
    set_text_if_changed(battery_label, battery_text);
    if ((int)demo != previous_demo) {
        if (demo) lv_obj_remove_flag(demo_label, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(demo_label, LV_OBJ_FLAG_HIDDEN);
        previous_demo = demo;
    }
    if (view.main == MAIN_FACE)
        f469_face_update(now_ms, state, view.mood, low_battery, view.manual);
    else
        f469_info_update(now_ms, state, diagnostics, view.info);
}
