#include "ui/f469_info.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "display_touch.h"
#include "ui/overview_model.h"
#include "ui/ui_theme.h"

enum { OVERVIEW_FIELDS = 9, DIAGNOSTIC_FIELDS = 11 };

static lv_obj_t *overview_page;
static lv_obj_t *diagnostic_page;
static lv_obj_t *overview_values[OVERVIEW_FIELDS];
static lv_obj_t *diagnostic_values[DIAGNOSTIC_FIELDS];
static lv_obj_t *brightness_slider;
static lv_obj_t *brightness_value;
static int previous_page = -1;

bool f469_info_brightness_hit_test(int32_t x, int32_t y)
{
    if (brightness_slider == NULL) return false;
    lv_area_t area;
    lv_obj_get_coords(brightness_slider, &area);
    return x >= area.x1 - 15 && x <= area.x2 + 15 &&
           y >= area.y1 - 15 && y <= area.y2 + 15;
}

static void set_text_if_changed(lv_obj_t *obj, const char *text)
{
    if (strcmp(lv_label_get_text(obj), text) != 0) lv_label_set_text(obj, text);
}

static void set_fmt_if_changed(lv_obj_t *obj, const char *format, ...)
{
    char text[64];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(text, sizeof text, format, args);
    va_end(args);
    set_text_if_changed(obj, text);
}

static void on_brightness_changed(lv_event_t *event)
{
    const int32_t percent = lv_slider_get_value(lv_event_get_target_obj(event));
    const uint8_t command = (uint8_t)((percent * 255 + 50) / 100);
    if (!f469_display_set_brightness(command)) return;
    set_fmt_if_changed(brightness_value, "%ld%%", (long)percent);
}

static lv_obj_t *make_text(lv_obj_t *parent, const char *text, int32_t x,
                            int32_t y, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static lv_obj_t *make_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(page, 0, 0);
    lv_obj_set_size(page, 800, 416);
    lv_obj_set_style_bg_color(page, UI_COLOR_BG, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    return page;
}

lv_obj_t *f469_info_create(lv_obj_t *parent)
{
    static const char *const overview_titles[OVERVIEW_FIELDS] = {
        "BATTERY", "THROTTLE", "STEERING", "MOTION", "PITCH", "ROLL",
        "GPS SAT", "ARMED", "LINK"
    };
    static const char *const diagnostic_titles[DIAGNOSTIC_FIELDS] = {
        "HEALTH", "FPS", "MSP AGE", "MSP TIMEOUTS", "UART OVERRUNS",
        "FRAME MISSES", "MAX LOOP", "WS BUSY DROPS", "LED EST.",
        "TOUCH", "RESET FLAGS"
    };
    lv_obj_t *container = make_page(parent);
    overview_page = make_page(container);
    diagnostic_page = make_page(container);
    lv_obj_remove_flag(overview_page, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(diagnostic_page, LV_OBJ_FLAG_CLICKABLE);
    make_text(overview_page, "OVERVIEW  1 / 2", 20, 13,
              &lv_font_montserrat_16, UI_COLOR_MUTED);
    make_text(diagnostic_page, "DIAGNOSTICS  2 / 2", 20, 13,
              &lv_font_montserrat_16, UI_COLOR_MUTED);

    for (unsigned i = 0; i < OVERVIEW_FIELDS; ++i) {
        const int32_t x = 16 + (int32_t)(i % 3u) * 261;
        const int32_t y = 50 + (int32_t)(i / 3u) * 102;
        lv_obj_t *tile = lv_obj_create(overview_page);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(tile, x, y);
        lv_obj_set_size(tile, 247, 90);
        ui_theme_apply_panel(tile);
        make_text(tile, overview_titles[i], 12, 8,
                  &lv_font_montserrat_16, UI_COLOR_MUTED);
        overview_values[i] = make_text(tile, "--", 12, 44,
                                       &lv_font_montserrat_24, UI_COLOR_YELLOW);
    }
    make_text(overview_page, "BRIGHTNESS", 20, 375,
              &lv_font_montserrat_16, UI_COLOR_MUTED);
    brightness_slider = lv_slider_create(overview_page);
    lv_obj_set_pos(brightness_slider, 185, 369);
    lv_obj_set_size(brightness_slider, 500, 30);
    lv_obj_set_ext_click_area(brightness_slider, 15);
    lv_slider_set_range(brightness_slider, 10, 100);
    lv_slider_set_value(brightness_slider, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(brightness_slider, UI_COLOR_YELLOW,
                              LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider, UI_COLOR_YELLOW,
                              LV_PART_KNOB);
    brightness_value = make_text(overview_page, "100%", 710, 375,
                                 &lv_font_montserrat_16, UI_COLOR_YELLOW);
    lv_obj_add_event_cb(brightness_slider, on_brightness_changed,
                        LV_EVENT_VALUE_CHANGED, NULL);
    for (unsigned i = 0; i < DIAGNOSTIC_FIELDS; ++i) {
        const int32_t x = 18 + (int32_t)(i / 6u) * 390;
        const int32_t y = 53 + (int32_t)(i % 6u) * 55;
        make_text(diagnostic_page, diagnostic_titles[i], x, y,
                  &lv_font_montserrat_16, UI_COLOR_MUTED);
        diagnostic_values[i] = make_text(diagnostic_page, "--", x + 206, y,
                                         &lv_font_montserrat_16, UI_COLOR_YELLOW);
    }
    lv_obj_add_flag(diagnostic_page, LV_OBJ_FLAG_HIDDEN);
    return container;
}

void f469_info_update(uint32_t now_ms, const VehicleState *state,
                       const DiagnosticsSnapshot *diag, InfoPage page)
{
    if (overview_page == NULL) return;
    if ((int)page != previous_page) {
        if (page == INFO_DIAGNOSTICS) {
            lv_obj_add_flag(overview_page, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(diagnostic_page, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(diagnostic_page, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(overview_page, LV_OBJ_FLAG_HIDDEN);
        }
        previous_page = page;
    }
    OverviewModel model;
    overview_model_from_state(state, now_ms, &model);
    const char *const fields[OVERVIEW_FIELDS] = {
        model.battery, model.throttle, model.steering, model.motion,
        model.pitch, model.roll, model.gps, model.armed, model.link
    };
    for (unsigned i = 0; i < OVERVIEW_FIELDS; ++i) {
        set_text_if_changed(overview_values[i], fields[i]);
    }
    if (diag == NULL) return;
    static const char *const health_names[] = {"BOOT", "OK", "DEGRADED", "FAULT"};
    set_text_if_changed(diagnostic_values[0],
        health_names[(unsigned)diag->state <= HEALTH_FAULT ? (unsigned)diag->state : 0u]);
    set_fmt_if_changed(diagnostic_values[1], "%u", (unsigned)diag->fps);
    set_fmt_if_changed(diagnostic_values[2], "%lu ms", (unsigned long)diag->msp_age_ms);
    set_fmt_if_changed(diagnostic_values[3], "%lu", (unsigned long)diag->msp_timeouts);
    set_fmt_if_changed(diagnostic_values[4], "%lu", (unsigned long)diag->uart_overruns);
    set_fmt_if_changed(diagnostic_values[5], "%lu", (unsigned long)diag->frame_misses);
    set_fmt_if_changed(diagnostic_values[6], "%lu us", (unsigned long)diag->max_loop_us);
    set_fmt_if_changed(diagnostic_values[7], "%lu", (unsigned long)diag->ws2812_busy_drops);
    set_fmt_if_changed(diagnostic_values[8], "%lu mA", (unsigned long)diag->led_current_ma);
    set_text_if_changed(diagnostic_values[9], diag->touch_available ? "YES" : "NO");
    set_fmt_if_changed(diagnostic_values[10], "0x%08lX", (unsigned long)diag->reset_flags);
}
