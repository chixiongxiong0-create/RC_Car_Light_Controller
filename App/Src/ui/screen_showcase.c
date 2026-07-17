#include "lvgl.h"
#include "ui/screen_showcase.h"

#include <stdio.h>

#include "ui/ui_theme.h"

static lv_obj_t *logo_group;
static lv_obj_t *slogan_group;
static lv_obj_t *armed_group;
static lv_obj_t *motion_group;
static lv_obj_t *motion_stripe;
static lv_obj_t *status_label;
static ShowcaseModel model;

static lv_obj_t *make_group(lv_obj_t *screen)
{
    lv_obj_t *group = lv_obj_create(screen);
    lv_obj_remove_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(group, 320, 208);
    lv_obj_set_pos(group, 0, 0);
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(group, 0, 0);
    lv_obj_set_style_pad_all(group, 0, 0);
    return group;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int32_t y,
                            lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_width(label, 310);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, y);
    return label;
}

lv_obj_t *screen_showcase_create(void)
{
    lv_obj_t *screen;
    lv_obj_t *label;
    ui_theme_init();
    screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    ui_theme_apply_screen(screen);

    logo_group = make_group(screen);
    label = make_label(logo_group, "WIO TRAIL SYSTEM", 52, UI_COLOR_YELLOW);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    make_label(logo_group, "CRAWLER H725", 94, UI_COLOR_MUTED);

    slogan_group = make_group(screen);
    make_label(slogan_group, "BUILT FOR SLOW LINES", 48, UI_COLOR_YELLOW);
    make_label(slogan_group, "AND HARD CLIMBS", 82, UI_COLOR_YELLOW);
    make_label(slogan_group, "/// TRAIL CONTROL SYSTEM ///", 130, UI_COLOR_MUTED);

    armed_group = make_group(screen);
    label = make_label(armed_group, "SYSTEM ARMED", 60, UI_COLOR_YELLOW);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    make_label(armed_group, "DRIVE OUTPUT LIVE", 104, lv_color_hex(0xF4F5F2));

    motion_group = make_group(screen);
    make_label(motion_group, "TRAIL MOTION", 30, UI_COLOR_YELLOW);
    motion_stripe = lv_obj_create(motion_group);
    lv_obj_remove_flag(motion_stripe, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(motion_stripe, 250, 34);
    lv_obj_align(motion_stripe, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_bg_color(motion_stripe, UI_COLOR_YELLOW, 0);
    lv_obj_set_style_bg_opa(motion_stripe, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(motion_stripe, 0, 0);
    lv_obj_set_style_radius(motion_stripe, 0, 0);
    make_label(motion_group, "KEEP THE LINE", 144, lv_color_hex(0xF4F5F2));

    status_label = lv_label_create(screen);
    lv_obj_set_size(status_label, 308, 24);
    lv_obj_set_pos(status_label, 6, 212);
    ui_theme_apply_caption(status_label);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    showcase_model_init(&model, 0u, false);
    return screen;
}

void screen_showcase_update(uint32_t now_ms, const VehicleState *state)
{
    char status[64];
    if (logo_group == NULL || state == NULL) {
        return;
    }
    model = showcase_model_update(model, state, now_ms);
    lv_obj_add_flag(logo_group, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(slogan_group, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(armed_group, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(motion_group, LV_OBJ_FLAG_HIDDEN);
    if (model.scene == SHOW_LOGO) {
        lv_obj_remove_flag(logo_group, LV_OBJ_FLAG_HIDDEN);
    } else if (model.scene == SHOW_SLOGAN) {
        lv_obj_remove_flag(slogan_group, LV_OBJ_FLAG_HIDDEN);
    } else if (model.scene == SHOW_ARMED) {
        lv_obj_remove_flag(armed_group, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(motion_group, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_set_style_transform_rotation(
        motion_stripe,
        model.stripe_direction == SHOW_STRIPE_LEFT ? -120 :
        model.stripe_direction == SHOW_STRIPE_RIGHT ? 120 : 0, 0);
    (void)snprintf(status, sizeof(status), "%s  RSSI %u  BAT %.1fV",
                   state->armed ? "ARMED" : "SAFE",
                   (unsigned)state->rssi, (double)state->battery_v);
    lv_label_set_text(status_label, status);
}
