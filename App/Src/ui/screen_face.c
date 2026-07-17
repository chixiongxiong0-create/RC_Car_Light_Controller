#include "lvgl.h"
#include "ui/screen_face.h"

#include "ui/ui_theme.h"

enum {
    FACE_UPDATE_MS = 33u,
    FACE_PULSE_PERIOD_MS = 1000u,
    FACE_BLINK_STEP_MS = 250u
};

static lv_obj_t *face_screen;
static lv_obj_t *left_eye;
static lv_obj_t *right_eye;
static lv_obj_t *left_core;
static lv_obj_t *right_core;
static uint32_t last_update_ms;
static uint32_t animation_phase_ms;
static bool updated;

static lv_obj_t *make_eye(lv_obj_t *parent, int32_t x, int32_t rotation,
                          lv_obj_t **core)
{
    lv_obj_t *eye = lv_obj_create(parent);
    lv_obj_remove_flag(eye, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(eye, x, 66);
    lv_obj_set_size(eye, 112, 104);
    lv_obj_set_style_bg_color(eye, lv_color_hex(0x202B33), 0);
    lv_obj_set_style_bg_opa(eye, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(eye, lv_color_hex(0x4A5C66), 0);
    lv_obj_set_style_border_width(eye, 4, 0);
    lv_obj_set_style_radius(eye, 8, 0);
    lv_obj_set_style_pad_all(eye, 0, 0);
    lv_obj_set_style_transform_rotation(eye, rotation, 0);

    *core = lv_obj_create(eye);
    lv_obj_remove_flag(*core, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(*core, 48, 72);
    lv_obj_align(*core, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(*core, UI_COLOR_YELLOW, 0);
    lv_obj_set_style_bg_opa(*core, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(*core, 0, 0);
    lv_obj_set_style_radius(*core, 4, 0);
    return eye;
}

lv_obj_t *screen_face_create(void)
{
    ui_theme_init();
    face_screen = lv_obj_create(NULL);
    lv_obj_remove_flag(face_screen, LV_OBJ_FLAG_SCROLLABLE);
    ui_theme_apply_screen(face_screen);
    left_eye = make_eye(face_screen, 38, -80, &left_core);
    right_eye = make_eye(face_screen, 170, 80, &right_core);
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
    const int32_t height = 30 + ((int32_t)model.aperture * 62) / 100;
    lv_color_t color = model.mood == FACE_REVERSE ?
                       lv_color_hex(0xF4F5F2) : UI_COLOR_YELLOW;
    lv_opa_t opacity = LV_OPA_COVER;

    if (model.mood == FACE_LOW_BATTERY) {
        const uint32_t half = FACE_PULSE_PERIOD_MS / 2u;
        const uint32_t ramp = animation_phase_ms < half ?
                              animation_phase_ms : FACE_PULSE_PERIOD_MS - animation_phase_ms;
        opacity = (lv_opa_t)(LV_OPA_40 + (ramp * (LV_OPA_COVER - LV_OPA_40)) / half);
    } else if (model.mood == FACE_LINK_LOST &&
               ((animation_phase_ms / FACE_BLINK_STEP_MS) & 1u) != 0u) {
        opacity = LV_OPA_TRANSP;
    }

    lv_obj_set_x(left_core, 32 + model.gaze_x);
    lv_obj_set_x(right_core, 32 + model.gaze_x);
    lv_obj_set_y(left_core, (104 - height) / 2);
    lv_obj_set_y(right_core, (104 - height) / 2);
    lv_obj_set_height(left_core, height);
    lv_obj_set_height(right_core, height);
    lv_obj_set_style_bg_color(left_core, color, 0);
    lv_obj_set_style_bg_color(right_core, color, 0);
    lv_obj_set_style_bg_opa(left_core, opacity, 0);
    lv_obj_set_style_bg_opa(right_core, opacity, 0);

    const int32_t rotation = model.mood == FACE_FOCUSED ? 130 : 80;
    lv_obj_set_style_transform_rotation(left_eye, -rotation, 0);
    lv_obj_set_style_transform_rotation(right_eye, rotation, 0);
}
