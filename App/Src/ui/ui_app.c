#include "ui/ui_app.h"

#include <stdio.h>

#include "lvgl.h"

static lv_obj_t *fps_label;
static lv_obj_t *moving_block;
static uint32_t last_frame_ms;
static uint32_t fps_window_ms;
static uint16_t frame_count;

void ui_app_init(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101214), 0);
    lv_obj_set_style_border_color(screen, lv_color_hex(0xf2c230), 0);
    lv_obj_set_style_border_width(screen, 3, 0);
    lv_obj_set_style_pad_all(screen, 8, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "WIO TRAIL SYSTEM");
    lv_obj_set_style_text_color(title, lv_color_hex(0xf2c230), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    fps_label = lv_label_create(screen);
    lv_label_set_text(fps_label, "0 FPS");
    lv_obj_set_style_text_color(fps_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_16, 0);
    lv_obj_align(fps_label, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    moving_block = lv_obj_create(screen);
    lv_obj_remove_flag(moving_block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(moving_block, 20, 20);
    lv_obj_set_style_radius(moving_block, 2, 0);
    lv_obj_set_style_border_width(moving_block, 0, 0);
    lv_obj_set_style_bg_color(moving_block, lv_color_hex(0xf2c230), 0);
    lv_obj_set_pos(moving_block, 8, 108);
}

void ui_app_tick(uint32_t now_ms, const VehicleState *state)
{
    (void)state;

    if ((uint32_t)(now_ms - last_frame_ms) >= 33u) {
        const int32_t travel = 320 - 16 - 20;
        const int32_t x = 8 + (int32_t)((now_ms / 8u) % (uint32_t)travel);
        lv_obj_set_x(moving_block, x);
        last_frame_ms = now_ms;
        ++frame_count;
    }

    if ((uint32_t)(now_ms - fps_window_ms) >= 1000u) {
        char text[10];
        unsigned fps = frame_count;
        if (fps > 99u) {
            fps = 99u;
        }
        (void)snprintf(text, sizeof(text), "%u FPS", fps);
        lv_label_set_text(fps_label, text);
        frame_count = 0u;
        fps_window_ms = now_ms;
    }
}
