#include "platform/touch_panel.h"

#include <stddef.h>

bool touch_panel_decode_ft(uint8_t status, const uint8_t point[4],
                           TouchSample *sample)
{
    if (sample == NULL) return false;
    sample->pressed = false;
    if ((status & 0x0fu) != 1u || point == NULL) return true;
    if ((point[0] >> 6u) == 1u || (point[2] >> 4u) == 0x0fu) return true;
    sample->x = (uint16_t)(((uint16_t)(point[0] & 0x0fu) << 8u) | point[1]);
    sample->y = (uint16_t)(((uint16_t)(point[2] & 0x0fu) << 8u) | point[3]);
    sample->pressed = true;
    return true;
}

bool touch_panel_decode_gt(uint8_t status, const uint8_t point[8],
                           TouchSample *sample)
{
    if (sample == NULL || (status & 0x80u) == 0u) return false;
    sample->pressed = false;
    if ((status & 0x0fu) != 1u || point == NULL) return true;
    sample->x = (uint16_t)((uint16_t)point[1] | ((uint16_t)point[2] << 8u));
    sample->y = (uint16_t)((uint16_t)point[3] | ((uint16_t)point[4] << 8u));
    sample->pressed = true;
    return true;
}

bool touch_panel_map_to_landscape(TouchSample *sample)
{
    if (sample == NULL || !sample->pressed) return sample != NULL;
    if (sample->x >= 240u || sample->y >= 320u) return false;
    const uint16_t physical_x = sample->x;
    sample->x = (uint16_t)(319u - sample->y);
    sample->y = physical_x;
    return true;
}

static bool read_resistive_adc(TouchProbeResult controller, TouchProbeRead read,
                               uint8_t command, void *ctx, uint16_t *value)
{
    uint8_t raw[2] = {0};
    if (!read(controller.address, command, false, raw, 2u, 5u, ctx) ||
        (raw[1] & 0x0fu) != 0u) {
        return false;
    }
    *value = (uint16_t)(((uint16_t)raw[0] << 4u) | (raw[1] >> 4u));
    return true;
}

TouchPollResult touch_panel_poll(TouchProbeResult controller,
                                 TouchProbeRead read, TouchPanelWrite write,
                                 void *ctx, TouchSample *sample)
{
    uint8_t status = 0u;
    uint8_t point[8] = {0};
    if (sample == NULL || read == NULL || controller.controller == TOUCH_NONE) {
        return TOUCH_POLL_IO_ERROR;
    }
    if (controller.controller == TOUCH_FT_FAMILY) {
        if (!read(controller.address, 0x02u, false, &status, 1u, 5u, ctx)) {
            return TOUCH_POLL_IO_ERROR;
        }
        if ((status & 0x0fu) == 1u &&
            !read(controller.address, 0x03u, false, point, 4u, 5u, ctx)) {
            return TOUCH_POLL_IO_ERROR;
        }
        (void)touch_panel_decode_ft(status, point, sample);
    } else if (controller.controller == TOUCH_GT_FAMILY) {
        const uint8_t clear = 0u;
        if (write == NULL ||
            !read(controller.address, 0x814eu, true, &status, 1u, 5u, ctx)) {
            return TOUCH_POLL_IO_ERROR;
        }
        if ((status & 0x80u) == 0u) return TOUCH_POLL_NO_DATA;
        if ((status & 0x0fu) == 1u &&
            !read(controller.address, 0x814fu, true, point, 8u, 5u, ctx)) {
            return TOUCH_POLL_IO_ERROR;
        }
        if (!write(controller.address, 0x814eu, true, &clear, 1u, 5u, ctx)) {
            return TOUCH_POLL_IO_ERROR;
        }
        (void)touch_panel_decode_gt(status, point, sample);
    } else if (controller.controller == TOUCH_RESISTIVE_FAMILY) {
        uint16_t z1 = 0u;
        if (!read_resistive_adc(controller, read, 0xE0u, ctx, &z1)) {
            return TOUCH_POLL_IO_ERROR;
        }
        sample->pressed = z1 >= 80u;
        if (sample->pressed) {
            uint16_t raw_x = 0u;
            uint16_t raw_y = 0u;
            if (!read_resistive_adc(controller, read, 0xC0u, ctx, &raw_x) ||
                !read_resistive_adc(controller, read, 0xD0u, ctx, &raw_y)) {
                return TOUCH_POLL_IO_ERROR;
            }
            sample->x = (uint16_t)((uint32_t)raw_x * 239u / 4095u);
            sample->y = (uint16_t)((uint32_t)raw_y * 319u / 4095u);
        }
    } else {
        return TOUCH_POLL_IO_ERROR;
    }
    if (!touch_panel_map_to_landscape(sample)) {
        sample->pressed = false;
    }
    return TOUCH_POLL_UPDATE;
}

#ifndef TOUCH_PROBE_HOST_TEST
#include "wio_lite_ai_bus.h"

static bool hal_touch_read(uint8_t address, uint16_t reg, bool reg16,
                           uint8_t *data, uint16_t length, uint32_t timeout_ms,
                           void *ctx)
{
    I2C_HandleTypeDef *i2c = ctx;
    return HAL_I2C_Mem_Read(i2c, (uint16_t)address << 1u, reg,
                            reg16 ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                            data, length, timeout_ms) == HAL_OK;
}

static bool hal_touch_write(uint8_t address, uint16_t reg, bool reg16,
                            const uint8_t *data, uint16_t length,
                            uint32_t timeout_ms, void *ctx)
{
    I2C_HandleTypeDef *i2c = ctx;
    return HAL_I2C_Mem_Write(i2c, (uint16_t)address << 1u, reg,
                             reg16 ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT,
                             (uint8_t *)data, length, timeout_ms) == HAL_OK;
}

TouchPollResult touch_panel_poll_hal(TouchProbeResult controller,
                                     TouchSample *sample)
{
    return touch_panel_poll(controller, hal_touch_read, hal_touch_write,
                            &hbus_i2c1, sample);
}
#endif
