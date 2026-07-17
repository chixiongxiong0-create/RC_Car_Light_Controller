#include "ui/screen_dashboard.h"

#include <stdio.h>
#include <string.h>

#include "ui/ui_theme.h"

static lv_obj_t *throttle_arc;
static lv_obj_t *throttle_value;
static lv_obj_t *direction_value;
static lv_obj_t *steering_value;
static lv_obj_t *battery_value;
static lv_obj_t *pitch_value;
static lv_obj_t *roll_value;
static lv_obj_t *gps_value;
static lv_obj_t *armed_value;
static lv_obj_t *mode_value;
static lv_obj_t *link_value;
static int displayed_throttle = 1000;

static void set_text_changed(lv_obj_t *label, const char *text)
{
    if (strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, bool value)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    if (value) {
        ui_theme_apply_value(label);
    } else {
        ui_theme_apply_caption(label);
    }
    return label;
}

static lv_obj_t *make_tile(lv_obj_t *parent, int32_t x, int32_t y,
                           const char *caption, lv_obj_t **value)
{
    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_size(tile, 84, 82);
    ui_theme_apply_panel(tile);
    lv_obj_t *label = make_label(tile, caption, false);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    *value = make_label(tile, "--", true);
    lv_obj_align(*value, LV_ALIGN_CENTER, 0, 8);
    return tile;
}

lv_obj_t *screen_dashboard_create(void)
{
    ui_theme_init();
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    ui_theme_apply_screen(screen);

    lv_obj_t *top = lv_obj_create(screen);
    lv_obj_remove_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_size(top, 320, 28);
    ui_theme_apply_top_status(top);
    armed_value = make_label(top, "SAFE", false);
    lv_obj_align(armed_value, LV_ALIGN_LEFT_MID, 0, 0);
    mode_value = make_label(top, "INAV", false);
    lv_obj_align(mode_value, LV_ALIGN_CENTER, 0, 0);
    link_value = make_label(top, "START", false);
    lv_obj_align(link_value, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_t *drive = lv_obj_create(screen);
    lv_obj_remove_flag(drive, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(drive, 4, 32);
    lv_obj_set_size(drive, 128, 204);
    ui_theme_apply_panel(drive);

    throttle_arc = lv_arc_create(drive);
    lv_obj_remove_flag(throttle_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(throttle_arc, 112, 112);
    lv_obj_align(throttle_arc, LV_ALIGN_TOP_MID, 0, 0);
    lv_arc_set_range(throttle_arc, -100, 100);
    lv_arc_set_bg_angles(throttle_arc, 135, 45);
    lv_arc_set_value(throttle_arc, 0);
    lv_obj_set_style_arc_color(throttle_arc, lv_color_hex(0x344651), LV_PART_MAIN);
    lv_obj_set_style_arc_color(throttle_arc, UI_COLOR_YELLOW, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(throttle_arc, 9, LV_PART_MAIN);
    lv_obj_set_style_arc_width(throttle_arc, 9, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(throttle_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(throttle_arc, 0, LV_PART_KNOB);

    throttle_value = make_label(drive, "0", true);
    lv_obj_align(throttle_value, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_t *throttle_caption = make_label(drive, "THROTTLE %", false);
    lv_obj_align(throttle_caption, LV_ALIGN_TOP_MID, 0, 69);
    direction_value = make_label(drive, "HOLD", false);
    lv_obj_set_style_text_color(direction_value, UI_COLOR_GREEN, 0);
    lv_obj_set_style_text_font(direction_value, &lv_font_montserrat_16, 0);
    lv_obj_align(direction_value, LV_ALIGN_TOP_MID, 0, 119);
    steering_value = make_label(drive, "STEER 0", false);
    lv_obj_set_style_text_color(steering_value, UI_COLOR_BLUE, 0);
    lv_obj_set_style_text_font(steering_value, &lv_font_montserrat_16, 0);
    lv_obj_align(steering_value, LV_ALIGN_TOP_MID, 0, 151);

    (void)make_tile(screen, 136, 32, "BATTERY", &battery_value);
    (void)make_tile(screen, 224, 32, "PITCH", &pitch_value);
    (void)make_tile(screen, 136, 120, "ROLL", &roll_value);
    (void)make_tile(screen, 224, 120, "GPS SAT", &gps_value);
    return screen;
}

void screen_dashboard_update(const VehicleState *state)
{
    char text[20];
    const int throttle = (int)(state->throttle * 100.0f);
    const int steering = (int)(state->steering * 100.0f);
    if (throttle != displayed_throttle) {
        lv_arc_set_value(throttle_arc, throttle);
        (void)snprintf(text, sizeof text, "%d", throttle);
        set_text_changed(throttle_value, text);
        displayed_throttle = throttle;
    }
    set_text_changed(direction_value, throttle > 3 ? "FORWARD" :
                     (throttle < -3 ? "REVERSE" : "HOLD"));
    (void)snprintf(text, sizeof text, "STEER %+d", steering);
    set_text_changed(steering_value, text);
    (void)snprintf(text, sizeof text, "%.1fV", (double)state->battery_v);
    set_text_changed(battery_value, text);
    (void)snprintf(text, sizeof text, "%+.1f", (double)state->pitch_deg);
    set_text_changed(pitch_value, text);
    (void)snprintf(text, sizeof text, "%+.1f", (double)state->roll_deg);
    set_text_changed(roll_value, text);
    (void)snprintf(text, sizeof text, "%u", (unsigned)state->gps_sats);
    set_text_changed(gps_value, text);
    set_text_changed(armed_value, state->armed ? "ARMED" : "SAFE");
    set_text_changed(mode_value, "INAV");
    switch (state->link) {
    case LINK_OK: set_text_changed(link_value, "LINK OK"); break;
    case LINK_STALE: set_text_changed(link_value, "STALE"); break;
    case LINK_LOST: set_text_changed(link_value, "LINK LOST"); break;
    default: set_text_changed(link_value, "START"); break;
    }
}
