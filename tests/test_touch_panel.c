#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "platform/touch_panel.h"

typedef struct {
    uint8_t address;
    uint8_t status;
    uint8_t point[8];
    bool fail_read;
    uint16_t raw_x;
    uint16_t raw_y;
    uint16_t raw_z1;
    unsigned reads;
    unsigned clears;
} FakePanel;

static bool fake_read(uint8_t address, uint16_t reg, bool reg16,
                      uint8_t *data, uint16_t length, uint32_t timeout_ms,
                      void *ctx)
{
    FakePanel *fake = ctx;
    assert(address == fake->address && timeout_ms == 5u);
    ++fake->reads;
    if (fake->fail_read) return false;
    if ((!reg16 && reg == 0x02u && length == 1u) ||
        (reg16 && reg == 0x814eu && length == 1u)) {
        data[0] = fake->status;
        return true;
    }
    if ((!reg16 && reg == 0x03u && length == 4u) ||
        (reg16 && reg == 0x814fu && length == 8u)) {
        for (uint16_t i = 0u; i < length; ++i) data[i] = fake->point[i];
        return true;
    }
    if (!reg16 && length == 2u &&
        (reg == 0xC0u || reg == 0xD0u || reg == 0xE0u)) {
        uint16_t raw = reg == 0xC0u ? fake->raw_x :
                       reg == 0xD0u ? fake->raw_y : fake->raw_z1;
        data[0] = (uint8_t)(raw >> 4u);
        data[1] = (uint8_t)(raw << 4u);
        return true;
    }
    assert(false);
    return false;
}

static bool fake_write(uint8_t address, uint16_t reg, bool reg16,
                       const uint8_t *data, uint16_t length,
                       uint32_t timeout_ms, void *ctx)
{
    FakePanel *fake = ctx;
    assert(address == fake->address && reg16 && reg == 0x814eu);
    assert(length == 1u && data[0] == 0u && timeout_ms == 5u);
    ++fake->clears;
    return true;
}

void test_touch_panel(void)
{
    TouchSample sample = {0};
    const uint8_t ft_point[4] = {0x00u, 100u, 0x00u, 200u};
    assert(touch_panel_decode_ft(1u, ft_point, &sample));
    assert(sample.pressed && sample.x == 100u && sample.y == 200u);
    assert(touch_panel_map_to_landscape(&sample));
    assert(sample.x == 119u && sample.y == 100u);

    assert(touch_panel_decode_ft(0u, NULL, &sample));
    assert(!sample.pressed);
    assert(touch_panel_decode_ft(2u, ft_point, &sample));
    assert(!sample.pressed);

    const uint8_t gt_point[8] = {0u, 100u, 0u, 200u, 0u, 0u, 0u, 0u};
    assert(touch_panel_decode_gt(0x81u, gt_point, &sample));
    assert(sample.pressed && sample.x == 100u && sample.y == 200u);
    assert(!touch_panel_decode_gt(0u, gt_point, &sample));
    assert(touch_panel_decode_gt(0x80u, NULL, &sample));
    assert(!sample.pressed);
    assert(touch_panel_decode_gt(0x82u, gt_point, &sample));
    assert(!sample.pressed);

    sample = (TouchSample){.pressed = true, .x = 240u, .y = 20u};
    assert(!touch_panel_map_to_landscape(&sample));
    sample = (TouchSample){.pressed = true, .x = 20u, .y = 320u};
    assert(!touch_panel_map_to_landscape(&sample));

    FakePanel ft = {.address = 0x38u, .status = 1u,
                    .point = {0u, 100u, 0u, 200u}};
    assert(touch_panel_poll((TouchProbeResult){TOUCH_FT_FAMILY, 0x38u},
                            fake_read, fake_write, &ft, &sample));
    assert(sample.pressed && sample.x == 119u && sample.y == 100u);
    assert(ft.reads == 2u && ft.clears == 0u);

    FakePanel gt = {.address = 0x14u, .status = 0x81u,
                    .point = {0u, 100u, 0u, 200u, 0u}};
    assert(touch_panel_poll((TouchProbeResult){TOUCH_GT_FAMILY, 0x14u},
                            fake_read, fake_write, &gt, &sample));
    assert(sample.pressed && sample.x == 119u && sample.y == 100u);
    assert(gt.reads == 2u && gt.clears == 1u);

    gt.status = 0x80u;
    gt.reads = gt.clears = 0u;
    assert(touch_panel_poll((TouchProbeResult){TOUCH_GT_FAMILY, 0x14u},
                            fake_read, fake_write, &gt, &sample));
    assert(!sample.pressed && gt.reads == 1u && gt.clears == 1u);

    gt.fail_read = true;
    assert(touch_panel_poll((TouchProbeResult){TOUCH_GT_FAMILY, 0x14u},
                            fake_read, fake_write, &gt, &sample) ==
           TOUCH_POLL_IO_ERROR);
    gt.fail_read = false;
    gt.status = 0u;
    assert(touch_panel_poll((TouchProbeResult){TOUCH_GT_FAMILY, 0x14u},
                            fake_read, fake_write, &gt, &sample) ==
           TOUCH_POLL_NO_DATA);

    FakePanel resistive = {.address = 0x48u, .raw_x = 2048u,
                           .raw_y = 2048u, .raw_z1 = 400u};
    assert(touch_panel_poll((TouchProbeResult){TOUCH_RESISTIVE_FAMILY, 0x48u},
                            fake_read, fake_write, &resistive, &sample) ==
           TOUCH_POLL_UPDATE);
    assert(sample.pressed && sample.x == 160u && sample.y == 119u);
    assert(resistive.reads == 3u);
    resistive.raw_z1 = 0u;
    assert(touch_panel_poll((TouchProbeResult){TOUCH_RESISTIVE_FAMILY, 0x48u},
                            fake_read, fake_write, &resistive, &sample) ==
           TOUCH_POLL_UPDATE);
    assert(!sample.pressed && resistive.reads == 4u);
}
