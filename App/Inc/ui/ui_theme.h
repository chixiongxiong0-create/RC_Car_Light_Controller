#pragma once

#include "lvgl.h"

#define UI_COLOR_BG       lv_color_hex(0x111820)
#define UI_COLOR_PANEL    lv_color_hex(0x1C2932)
#define UI_COLOR_YELLOW   lv_color_hex(0xFFB000)
#define UI_COLOR_GREEN    lv_color_hex(0x41D18B)
#define UI_COLOR_BLUE     lv_color_hex(0x5FB9FF)
#define UI_COLOR_MUTED    lv_color_hex(0x93A7B1)

void ui_theme_init(void);
void ui_theme_apply_screen(lv_obj_t *obj);
void ui_theme_apply_panel(lv_obj_t *obj);
void ui_theme_apply_value(lv_obj_t *obj);
void ui_theme_apply_caption(lv_obj_t *obj);
void ui_theme_apply_warning(lv_obj_t *obj);
void ui_theme_apply_top_status(lv_obj_t *obj);
