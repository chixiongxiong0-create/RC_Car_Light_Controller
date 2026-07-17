#include "ui/ui_theme.h"

static lv_style_t screen_style;
static lv_style_t panel_style;
static lv_style_t value_style;
static lv_style_t caption_style;
static lv_style_t warning_style;
static lv_style_t top_status_style;
static bool initialized;

void ui_theme_init(void)
{
    if (initialized) {
        return;
    }

    lv_style_init(&screen_style);
    lv_style_set_bg_color(&screen_style, UI_COLOR_BG);
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);
    lv_style_set_pad_all(&screen_style, 0);

    lv_style_init(&panel_style);
    lv_style_set_bg_color(&panel_style, UI_COLOR_PANEL);
    lv_style_set_bg_opa(&panel_style, LV_OPA_COVER);
    lv_style_set_border_color(&panel_style, lv_color_hex(0x31434E));
    lv_style_set_border_width(&panel_style, 1);
    lv_style_set_radius(&panel_style, 3);
    lv_style_set_pad_all(&panel_style, 5);

    lv_style_init(&value_style);
    lv_style_set_text_color(&value_style, UI_COLOR_YELLOW);
    lv_style_set_text_font(&value_style, &lv_font_montserrat_24);

    lv_style_init(&caption_style);
    lv_style_set_text_color(&caption_style, UI_COLOR_MUTED);
    lv_style_set_text_font(&caption_style, &lv_font_montserrat_12);

    lv_style_init(&warning_style);
    lv_style_set_text_color(&warning_style, UI_COLOR_YELLOW);
    lv_style_set_text_font(&warning_style, &lv_font_montserrat_16);

    lv_style_init(&top_status_style);
    lv_style_set_bg_color(&top_status_style, lv_color_hex(0x0B1117));
    lv_style_set_bg_opa(&top_status_style, LV_OPA_COVER);
    lv_style_set_border_width(&top_status_style, 0);
    lv_style_set_radius(&top_status_style, 0);
    lv_style_set_pad_left(&top_status_style, 6);
    lv_style_set_pad_right(&top_status_style, 6);
    lv_style_set_text_color(&top_status_style, UI_COLOR_MUTED);
    lv_style_set_text_font(&top_status_style, &lv_font_montserrat_12);
    initialized = true;
}

void ui_theme_apply_screen(lv_obj_t *obj) { lv_obj_add_style(obj, &screen_style, 0); }
void ui_theme_apply_panel(lv_obj_t *obj) { lv_obj_add_style(obj, &panel_style, 0); }
void ui_theme_apply_value(lv_obj_t *obj) { lv_obj_add_style(obj, &value_style, 0); }
void ui_theme_apply_caption(lv_obj_t *obj) { lv_obj_add_style(obj, &caption_style, 0); }
void ui_theme_apply_warning(lv_obj_t *obj) { lv_obj_add_style(obj, &warning_style, 0); }
void ui_theme_apply_top_status(lv_obj_t *obj) { lv_obj_add_style(obj, &top_status_style, 0); }
