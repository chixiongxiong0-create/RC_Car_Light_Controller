#include "platform/touch_probe.h"

#include <stddef.h>
#include <string.h>

TouchController touch_probe_identify(TouchProbeRead read, void *ctx)
{
    uint8_t id[4] = {0};
    if (read == NULL) {
        return TOUCH_NONE;
    }
    static const uint8_t ft_chip_ids[] = {
        0x03u, 0x06u, 0x0Au, 0x11u, 0x36u, 0x54u, 0x64u
    };
    static const uint8_t ft_vendor_ids[] = {0x11u, 0x79u};
    uint8_t chip_id = 0u;
    uint8_t vendor_id = 0u;
    bool known_chip = false;
    bool known_vendor = false;
    if (read(0x38u, 0xA3u, false, &chip_id, 1u, 5u, ctx) &&
        read(0x38u, 0xA8u, false, &vendor_id, 1u, 5u, ctx)) {
        for (size_t i = 0u; i < sizeof ft_chip_ids; ++i) {
            known_chip |= chip_id == ft_chip_ids[i];
        }
        for (size_t i = 0u; i < sizeof ft_vendor_ids; ++i) {
            known_vendor |= vendor_id == ft_vendor_ids[i];
        }
    }
    if (known_chip && known_vendor) {
        return TOUCH_FT_FAMILY;
    }
    memset(id, 0, sizeof id);
    if (read(0x5Du, 0x8140u, true, id, 4u, 5u, ctx) &&
        id[0] == '9' && id[1] == '1') {
        return TOUCH_GT_FAMILY;
    }
    memset(id, 0, sizeof id);
    if (read(0x14u, 0x8140u, true, id, 4u, 5u, ctx) &&
        id[0] == '9' && id[1] == '1') {
        return TOUCH_GT_FAMILY;
    }
    return TOUCH_NONE;
}

#ifndef TOUCH_PROBE_HOST_TEST
#include "wio_lite_ai_bus.h"

static bool hal_read(uint8_t address, uint16_t reg, bool reg16,
                     uint8_t *data, uint16_t length, uint32_t timeout_ms,
                     void *ctx)
{
    I2C_HandleTypeDef *i2c = ctx;
    const uint16_t mem_size = reg16 ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;
    return HAL_I2C_Mem_Read(i2c, (uint16_t)address << 1u, reg, mem_size,
                            data, length, timeout_ms) == HAL_OK;
}

TouchController touch_probe_boot(void)
{
    if (BSP_I2C4_Init() != BSP_ERROR_NONE) {
        return TOUCH_NONE;
    }
    return touch_probe_identify(hal_read, &hbus_i2c4);
}
#endif
