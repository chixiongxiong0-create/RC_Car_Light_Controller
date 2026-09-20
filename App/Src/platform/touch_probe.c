#include "platform/touch_probe.h"

#include <stddef.h>
#include <string.h>

TouchProbeResult touch_probe_detect(TouchProbeRead read, void *ctx)
{
    uint8_t id[4] = {0};
    if (read == NULL) {
        return (TouchProbeResult){TOUCH_NONE, 0u};
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
        return (TouchProbeResult){TOUCH_FT_FAMILY, 0x38u};
    }
    memset(id, 0, sizeof id);
    if (read(0x5Du, 0x8140u, true, id, 4u, 5u, ctx) &&
        id[0] == '9' && id[1] == '1') {
        return (TouchProbeResult){TOUCH_GT_FAMILY, 0x5du};
    }
    memset(id, 0, sizeof id);
    if (read(0x14u, 0x8140u, true, id, 4u, 5u, ctx) &&
        id[0] == '9' && id[1] == '1') {
        return (TouchProbeResult){TOUCH_GT_FAMILY, 0x14u};
    }
    /* The Wio resistive LCD responds at 0x48 with NS2009-compatible ADC commands. */
    uint8_t z1[2] = {0};
    if (read(0x48u, 0xE0u, false, z1, 2u, 5u, ctx) &&
        (z1[1] & 0x0fu) == 0u) {
        return (TouchProbeResult){TOUCH_RESISTIVE_FAMILY, 0x48u};
    }
    return (TouchProbeResult){TOUCH_NONE, 0u};
}

TouchController touch_probe_identify(TouchProbeRead read, void *ctx)
{
    return touch_probe_detect(read, ctx).controller;
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

TouchProbeResult touch_probe_boot_detect(void)
{
    /* The LCD FPC touch lines are on I2C1 (PB6/PB7), not I2C4. */
    if (BSP_I2C1_Init() != BSP_ERROR_NONE) {
        return (TouchProbeResult){TOUCH_NONE, 0u};
    }
    return touch_probe_detect(hal_read, &hbus_i2c1);
}

TouchController touch_probe_boot(void)
{
    return touch_probe_boot_detect().controller;
}
#endif
