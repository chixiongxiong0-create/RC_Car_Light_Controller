#include "lvgl.h"
#include "ui/screen_face.h"

#include <stdio.h>

#include "app_config.h"
#include "platform/lvgl_port.h"
#include "ui/ui_theme.h"

enum {
    FACE_UPDATE_MS = 33u,
    FACE_PULSE_PERIOD_MS = 1000u,
    BATTERY_SEGMENT_COUNT = 6u
};

static lv_obj_t *face_screen;
static lv_obj_t *left_eye;
static lv_obj_t *right_eye;
static lv_obj_t *left_core;
static lv_obj_t *right_core;
static lv_obj_t *mouth;
static lv_obj_t *battery_body;
static lv_obj_t *battery_tip;
static lv_obj_t *battery_segments[BATTERY_SEGMENT_COUNT];
static lv_obj_t *battery_voltage;
static uint32_t last_update_ms;
static uint32_t animation_phase_ms;
static bool updated;

static void style_plain(lv_obj_t *obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static lv_obj_t *make_eye(lv_obj_t *parent, lv_obj_t **core)
{
    lv_obj_t *eye = lv_obj_create(parent);
    style_plain(eye);
    lv_obj_set_size(eye, 100, 108);
    lv_obj_set_style_bg_color(eye, lv_color_hex(0x182128), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(eye, lv_color_hex(0x566A75), 0);
    lv_obj_set_style_border_width(eye, 3, 0);
    lv_obj_set_style_radius(eye, LV_RADIUS_CIRCLE, 0);

    *core = lv_obj_create(eye);
    style_plain(*core);
    lv_obj_set_size(*core, 42, 42);
    lv_obj_align(*core, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(*core, UI_COLOR_YELLOW, 0);
    lv_obj_set_style_bg_opa(*core, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(*core, LV_RADIUS_CIRCLE, 0);
    return eye;
}

static void make_battery(lv_obj_t *parent)
{
    battery_body = lv_obj_create(parent);
    style_plain(battery_body);
    lv_obj_set_pos(battery_body, 86, 16);
    lv_obj_set_size(battery_body, 92, 30);
    lv_obj_set_style_bg_opa(battery_body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(battery_body, UI_COLOR_GREEN, 0);
    lv_obj_set_style_border_width(battery_body, 3, 0);
    lv_obj_set_style_radius(battery_body, 6, 0);
    lv_obj_set_flex_flow(battery_body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(battery_body, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(battery_body, 4, 0);
    lv_obj_set_style_pad_right(battery_body, 4, 0);

    for (uint32_t i = 0u; i < BATTERY_SEGMENT_COUNT; ++i) {
        battery_segments[i] = lv_obj_create(battery_body);
        style_plain(battery_segments[i]);
        lv_obj_set_size(battery_segments[i], 10, 18);
        lv_obj_set_style_radius(battery_segments[i], 2, 0);
        lv_obj_set_style_bg_color(battery_segments[i], UI_COLOR_GREEN, 0);
        lv_obj_set_style_bg_opa(battery_segments[i], LV_OPA_TRANSP, 0);
    }

    battery_tip = lv_obj_create(parent);
    style_plain(battery_tip);
    lv_obj_set_pos(battery_tip, 178, 24);
    lv_obj_set_size(battery_tip, 6, 14);
    lv_obj_set_style_radius(battery_tip, 0, 0);
    lv_obj_set_style_bg_color(battery_tip, UI_COLOR_GREEN, 0);
    lv_obj_set_style_bg_opa(battery_tip, LV_OPA_COVER, 0);

    battery_voltage = lv_label_create(parent);
    lv_obj_set_pos(battery_voltage, 192, 20);
    lv_obj_set_width(battery_voltage, 72);
    lv_obj_set_style_text_color(battery_voltage, UI_COLOR_GREEN, 0);
    lv_obj_set_style_text_font(battery_voltage, &lv_font_montserrat_16, 0);
    lv_label_set_text(battery_voltage, "--.-V");
}

static void set_mouth(FaceMood mood)
{
    lv_color_t color = lv_color_hex(0x71858F);
    int32_t width = 26;
    int32_t height = 7;
    int32_t radius = LV_RADIUS_CIRCLE;

    if (mood == FACE_FOCUSED) {
        width = 30;
        height = 24;
        color = lv_color_hex(0xF08F96);
    } else if (mood == FACE_REVERSE) {
        width = 34;
        height = 5;
        radius = 2;
    } else if (mood == FACE_LOW_BATTERY) {
        width = 22;
        height = 5;
        radius = 2;
    } else if (mood == FACE_LINK_LOST) {
        width = 38;
        height = 5;
        color = lv_color_hex(0xFF686F);
        radius = 2;
    }
    lv_obj_set_size(mouth, width, height);
    lv_obj_set_pos(mouth, ((int32_t)UI_WIDTH - width) / 2,
                   (int32_t)UI_HEIGHT - 30);
    lv_obj_set_style_radius(mouth, radius, 0);
    lv_obj_set_style_bg_color(mouth, color, 0);
}

static void update_battery(uint32_t now_ms, const VehicleState *state,
                           bool low_battery)
{
    const FaceBatteryModel model = face_battery_model(
        state->battery_v, state->battery_valid, APP_BATTERY_CELL_COUNT);
    const bool warning = model.low || low_battery;
    const bool dim = warning && (now_ms % FACE_PULSE_PERIOD_MS) >=
                                (FACE_PULSE_PERIOD_MS / 2u);
    const lv_color_t color = warning ? lv_color_hex(0xFF525D) : UI_COLOR_GREEN;
    const lv_opa_t opacity = dim ? LV_OPA_30 : LV_OPA_COVER;
    char voltage_text[16];

    if (state->battery_valid && state->battery_v > 0.0f) {
        (void)snprintf(voltage_text, sizeof voltage_text, "%.1fV",
                       (double)state->battery_v);
    } else {
        (void)snprintf(voltage_text, sizeof voltage_text, "--.-V");
    }
    lv_label_set_text(battery_voltage, voltage_text);
    lv_obj_set_style_text_color(battery_voltage, color, 0);
    lv_obj_set_style_text_opa(battery_voltage, opacity, 0);
    lv_obj_set_style_border_color(battery_body, color, 0);
    lv_obj_set_style_border_opa(battery_body, opacity, 0);
    lv_obj_set_style_bg_color(battery_tip, color, 0);
    lv_obj_set_style_bg_opa(battery_tip, opacity, 0);

    for (uint32_t i = 0u; i < BATTERY_SEGMENT_COUNT; ++i) {
        lv_obj_set_style_bg_color(battery_segments[i], color, 0);
        lv_obj_set_style_bg_opa(battery_segments[i],
                                i < model.segments ? opacity : LV_OPA_TRANSP, 0);
    }
}

lv_obj_t *screen_face_create(void)
{
    ui_theme_init();
    face_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(face_screen, LV_OBJ_FLAG_SCROLLABLE);
    ui_theme_apply_screen(face_screen);

    make_battery(face_screen);
    left_eye = make_eye(face_screen, &left_core);
    right_eye = make_eye(face_screen, &right_core);
    lv_obj_set_pos(left_eye, 48, 92);
    lv_obj_set_pos(right_eye, 172, 92);

    mouth = lv_obj_create(face_screen);
    style_plain(mouth);
    lv_obj_set_style_bg_opa(mouth, LV_OPA_COVER, 0);
    set_mouth(FACE_IDLE);

    updated = false;
    last_update_ms = 0u;
    animation_phase_ms = 0u;
    return face_screen;
}

void screen_face_update(uint32_t now_ms, const VehicleState *state,
                        bool low_battery)
{
    if (face_screen == NULL || state == NULL) {
        return;
    }
    const uint32_t elapsed_ms = (uint32_t)(now_ms - last_update_ms);
    if (updated && elapsed_ms < FACE_UPDATE_MS) {
        return;
    }
    animation_phase_ms = (animation_phase_ms + elapsed_ms) % FACE_PULSE_PERIOD_MS;
    last_update_ms = now_ms;
    updated = true;

    const FaceModel model = face_model_from_state(state, low_battery);
    const int32_t energy = model.aperture > 35u ? model.aperture - 35u : 0u;
    const int32_t eye_growth = (energy * 12) / 65;
    const int32_t eye_width = 100 + eye_growth;
    const int32_t eye_height = 108 + eye_growth;
    const int32_t core_size = 36 + ((int32_t)model.aperture * 20) / 100;
    const FaceMotionModel motion = face_motion_model(now_ms, model.aperture,
                                                      model.mood);
    const int32_t visible_eye_height = eye_height -
        ((eye_height - 12) * motion.blink_closure) / 100;
    const int32_t core_height = core_size -
        ((core_size - 4) * motion.blink_closure) / 100;
    const lv_color_t color = model.mood == FACE_REVERSE ?
                             lv_color_hex(0xF4F5F2) : UI_COLOR_YELLOW;

    lv_obj_set_size(left_eye, eye_width, visible_eye_height);
    lv_obj_set_size(right_eye, eye_width, visible_eye_height);
    lv_obj_set_pos(left_eye, 48 - eye_growth / 2 + motion.shake_x,
                   92 - eye_growth / 2 + motion.shake_y +
                   (eye_height - visible_eye_height) / 2);
    lv_obj_set_pos(right_eye, 172 - eye_growth / 2 + motion.shake_x,
                   92 - eye_growth / 2 - motion.shake_y +
                   (eye_height - visible_eye_height) / 2);

    lv_obj_set_size(left_core, core_size, core_height);
    lv_obj_set_size(right_core, core_size, core_height);
    lv_obj_align(left_core, LV_ALIGN_CENTER, model.gaze_x, -energy / 16);
    lv_obj_align(right_core, LV_ALIGN_CENTER, model.gaze_x, -energy / 16);
    lv_obj_set_style_bg_color(left_core, color, 0);
    lv_obj_set_style_bg_color(right_core, color, 0);
    lv_obj_set_style_radius(left_core, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(right_core, LV_RADIUS_CIRCLE, 0);

    if (model.mood == FACE_LOW_BATTERY) {
        const lv_opa_t eye_opa = animation_phase_ms < 500u ? LV_OPA_COVER : LV_OPA_50;
        lv_obj_set_style_bg_opa(left_core, eye_opa, 0);
        lv_obj_set_style_bg_opa(right_core, eye_opa, 0);
    } else {
        lv_obj_set_style_bg_opa(left_core, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_opa(right_core, LV_OPA_COVER, 0);
    }

    set_mouth(model.mood);
    update_battery(now_ms, state, low_battery);
}
