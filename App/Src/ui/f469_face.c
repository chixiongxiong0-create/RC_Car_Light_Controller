#include "ui/f469_face.h"

#include <stdio.h>
#include <string.h>
#include "ui/ui_theme.h"

static lv_obj_t *left_eye;
static lv_obj_t *right_eye;
static lv_obj_t *left_pupil;
static lv_obj_t *right_pupil;
static lv_obj_t *mouth;
static lv_obj_t *mood_label;
static lv_obj_t *mode_label;
static lv_obj_t *voltage_label;
static int previous_mood = -1;
static int previous_low_battery = -1;
static int32_t previous_eye_height = -1;
static int32_t previous_gaze = INT32_MIN;
static int32_t previous_pupil_y = INT32_MIN;

static void set_text_if_changed(lv_obj_t *obj, const char *text)
{
    if (strcmp(lv_label_get_text(obj), text) != 0) lv_label_set_text(obj, text);
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int32_t x, int32_t y,
                       const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    return obj;
}

static lv_obj_t *eye(lv_obj_t *parent, int32_t x, lv_obj_t **pupil)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(obj, x, 120);
    lv_obj_set_size(obj, 165, 170);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1D303B), 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x65818B), 0);
    lv_obj_set_style_border_width(obj, 4, 0);
    *pupil = lv_obj_create(obj);
    lv_obj_remove_flag(*pupil, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(*pupil, 66, 66);
    lv_obj_set_style_radius(*pupil, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(*pupil, 0, 0);
    lv_obj_set_style_bg_color(*pupil, UI_COLOR_YELLOW, 0);
    lv_obj_align(*pupil, LV_ALIGN_CENTER, 0, 0);
    return obj;
}

lv_obj_t *f469_face_create(lv_obj_t *parent)
{
    lv_obj_t *screen = lv_obj_create(parent);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(screen, 0, 0);
    lv_obj_set_size(screen, 800, 416);
    lv_obj_set_style_bg_color(screen, UI_COLOR_BG, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_radius(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    mood_label = label(screen, "CALM", 35, 22, &lv_font_montserrat_24,
                       UI_COLOR_YELLOW);
    mode_label = label(screen, "AUTO", 690, 22, &lv_font_montserrat_16,
                       UI_COLOR_MUTED);
    left_eye = eye(screen, 190, &left_pupil);
    right_eye = eye(screen, 445, &right_pupil);

    mouth = lv_obj_create(screen);
    lv_obj_remove_flag(mouth, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(mouth, 365, 337);
    lv_obj_set_size(mouth, 70, 10);
    lv_obj_set_style_bg_color(mouth, UI_COLOR_MUTED, 0);
    lv_obj_set_style_border_width(mouth, 0, 0);
    lv_obj_set_style_radius(mouth, LV_RADIUS_CIRCLE, 0);

    voltage_label = label(screen, "--.-V", 35, 365,
                          &lv_font_montserrat_24, UI_COLOR_GREEN);
    label(screen, "SWIPE TO PREVIEW", 535, 370, &lv_font_montserrat_16,
          UI_COLOR_MUTED);
    return screen;
}

void f469_face_update(uint32_t now_ms, const VehicleState *state,
                       FaceMood mood, bool low_battery, bool manual)
{
    if (state == NULL || left_eye == NULL) return;
    static const char *const names[] = {
        "CALM", "FOCUSED", "REVERSE", "LOW POWER", "LINK LOST"
    };
    set_text_if_changed(mood_label, names[(unsigned)mood <= FACE_LINK_LOST ?
                                          (unsigned)mood : 0u]);
    set_text_if_changed(mode_label, manual ? "MANUAL" : "AUTO");
    const FaceModel base = face_model_from_state(state, low_battery);
    const FaceMotionModel motion = face_motion_model(now_ms, base.aperture, mood);
    const int32_t eye_height = 170 - (motion.blink_closure * 150) / 100;
    const bool geometry_changed = eye_height != previous_eye_height;
    if (geometry_changed) {
        lv_obj_set_size(left_eye, 165, eye_height);
        lv_obj_set_size(right_eye, 165, eye_height);
        previous_eye_height = eye_height;
    }
    if (geometry_changed) {
        const int32_t eye_y = 120 + (170 - eye_height) / 2;
        lv_obj_set_pos(left_eye, 190, eye_y);
        lv_obj_set_pos(right_eye, 445, eye_y);
    }
    const int32_t gaze = base.gaze_x + motion.gaze_offset_x + motion.shake_x;
    const int32_t pupil_y = motion.pupil_offset_y + motion.shake_y;
    if (gaze != previous_gaze || pupil_y != previous_pupil_y) {
        lv_obj_align(left_pupil, LV_ALIGN_CENTER, gaze, pupil_y);
        lv_obj_align(right_pupil, LV_ALIGN_CENTER, gaze, pupil_y);
        previous_gaze = gaze;
        previous_pupil_y = pupil_y;
    }
    const lv_color_t color = mood == FACE_LINK_LOST || mood == FACE_LOW_BATTERY ?
                             lv_color_hex(0xFF646C) :
                             mood == FACE_REVERSE ? lv_color_white() : UI_COLOR_YELLOW;
    if ((int)mood != previous_mood) {
        lv_obj_set_style_bg_color(left_pupil, color, 0);
        lv_obj_set_style_bg_color(right_pupil, color, 0);
        lv_obj_set_style_text_color(mood_label, color, 0);
    }
    const int32_t mouth_width = mood == FACE_FOCUSED ? 55 :
                                mood == FACE_LINK_LOST ? 90 : 70;
    const int32_t mouth_height = mood == FACE_FOCUSED ? 32 : 10;
    if ((int)mood != previous_mood) {
        lv_obj_set_size(mouth, mouth_width, mouth_height);
        lv_obj_set_pos(mouth, (800 - mouth_width) / 2, 337);
        lv_obj_set_style_bg_color(mouth, color, 0);
        previous_mood = mood;
    }
    char voltage[16];
    if (state->battery_valid && state->link == LINK_OK) {
        (void)snprintf(voltage, sizeof voltage, "%.1fV", (double)state->battery_v);
    } else {
        (void)snprintf(voltage, sizeof voltage, "--.-V");
    }
    set_text_if_changed(voltage_label, voltage);
    if ((int)low_battery != previous_low_battery) {
        lv_obj_set_style_text_color(voltage_label,
            low_battery ? lv_color_hex(0xFF646C) : UI_COLOR_GREEN, 0);
        previous_low_battery = low_battery;
    }
}
