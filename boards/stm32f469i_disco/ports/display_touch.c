#include "display_touch.h"

#include <string.h>

#include "lvgl.h"
#include "stm32469i_discovery_lcd.h"
#include "stm32469i_discovery_ts.h"
#include "ft6x06.h"
#include "ui/f469_ui.h"

extern DSI_HandleTypeDef hdsi_eval;

enum { LCD_WIDTH = 800, LCD_HEIGHT = 480, DRAW_LINES = 40,
       DCS_WRITE_DISPLAY_BRIGHTNESS = 0x51 };

static uint16_t draw_buffer[LCD_WIDTH * DRAW_LINES];
static lv_display_t *display;
static bool touch_ok;
static uint8_t touch_address;
static uint32_t last_tick_ms;
static bool gesture_tracking;
static int32_t gesture_start_x;
static int32_t gesture_start_y;
static int32_t gesture_last_x;
static int32_t gesture_last_y;
static bool gesture_was_pressed;

static void track_swipe(const lv_indev_data_t *data)
{
    if (data->state == LV_INDEV_STATE_PRESSED) {
        if (!gesture_was_pressed) {
            gesture_start_x = data->point.x;
            gesture_start_y = data->point.y;
            gesture_tracking = data->point.y >= 64;
        }
        gesture_last_x = data->point.x;
        gesture_last_y = data->point.y;
        gesture_was_pressed = true;
    } else if (gesture_was_pressed) {
        const int32_t dx = gesture_last_x - gesture_start_x;
        const int32_t dy = gesture_last_y - gesture_start_y;
        if (gesture_tracking && (dx >= 70 || dx <= -70) &&
            dy > -100 && dy < 100) {
            f469_ui_swipe(dx < 0, lv_tick_get());
        }
        gesture_was_pressed = false;
        gesture_tracking = false;
    }
}

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *pixels)
{
    uint32_t *const scanout = (uint32_t *)LCD_FB_START_ADDRESS;
    const uint16_t *src = (const uint16_t *)pixels;
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            const uint16_t rgb565 = *src++;
            const uint32_t r = ((rgb565 >> 11) & 31u) * 255u / 31u;
            const uint32_t g = ((rgb565 >> 5) & 63u) * 255u / 63u;
            const uint32_t b = (rgb565 & 31u) * 255u / 31u;
            scanout[(uint32_t)y * LCD_WIDTH + (uint32_t)x] =
                0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
    lv_display_flush_ready(disp);
}

static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    TS_StateTypeDef state = {0};
    if (!touch_ok || BSP_TS_GetState(&state) != TS_OK ||
        state.touchDetected == 0u) {
        data->state = LV_INDEV_STATE_RELEASED;
        track_swipe(data);
        return;
    }
    data->point.x = state.touchX[0];
    data->point.y = state.touchY[0];
    data->state = LV_INDEV_STATE_PRESSED;
    track_swipe(data);
}

bool f469_display_touch_init(void)
{
    if (BSP_LCD_Init() != LCD_OK) return false;
    if (BSP_LCD_GetXSize() != LCD_WIDTH ||
        BSP_LCD_GetYSize() != LCD_HEIGHT) return false;
    BSP_LCD_LayerDefaultInit(0u, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0u);
    memset((void *)LCD_FB_START_ADDRESS, 0, LCD_WIDTH * LCD_HEIGHT * 4u);
    BSP_LCD_DisplayOn();
    if (HAL_DSI_ShortWrite(&hdsi_eval, LCD_Driver_ID,
                           DSI_DCS_SHORT_PKT_WRITE_P1,
                           DCS_WRITE_DISPLAY_BRIGHTNESS, 0xFFu) != HAL_OK) return false;

    lv_init();
    display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    if (display == NULL) return false;
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, NULL, sizeof draw_buffer,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_cb);

    touch_ok = BSP_TS_Init(LCD_WIDTH, LCD_HEIGHT) == TS_OK;
    if (touch_ok) {
        uint8_t id = TS_IO_Read(TS_I2C_ADDRESS, FT6206_CHIP_ID_REG);
        if (id == FT6206_ID_VALUE) touch_address = TS_I2C_ADDRESS;
        else {
            id = TS_IO_Read(TS_I2C_ADDRESS_A02, FT6206_CHIP_ID_REG);
            if (id == FT6206_ID_VALUE || id == FT6X36_ID2_VALUE)
                touch_address = TS_I2C_ADDRESS_A02;
            else touch_ok = false;
        }
    }
    if (touch_ok) {
        lv_indev_t *indev = lv_indev_create();
        if (indev == NULL) touch_ok = false;
        else {
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(indev, touch_cb);
            lv_timer_set_period(lv_indev_get_read_timer(indev), 20u);
        }
    }
    last_tick_ms = HAL_GetTick();
    return true;
}

void f469_display_touch_tick(uint32_t now_ms)
{
    lv_tick_inc(now_ms - last_tick_ms);
    last_tick_ms = now_ms;
    lv_timer_handler();
}

bool f469_touch_available(void)
{
    return touch_ok;
}

void f469_touch_health_check(void)
{
    if (!touch_ok) return;
    const uint8_t id = TS_IO_Read(touch_address, FT6206_CHIP_ID_REG);
    if (id != FT6206_ID_VALUE && id != FT6X36_ID2_VALUE) touch_ok = false;
}
