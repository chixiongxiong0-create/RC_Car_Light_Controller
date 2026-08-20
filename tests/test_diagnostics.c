#include <assert.h>
#include <string.h>

#include "diagnostics.h"
#include "platform/touch_probe.h"

typedef struct {
    unsigned calls;
    uint8_t hit_address;
    uint8_t chip_id;
    uint8_t vendor_id;
    uint8_t gt_id[4];
} ProbeFake;

static bool fake_read(uint8_t address, uint16_t reg, bool reg16,
                      uint8_t *data, uint16_t length, uint32_t timeout_ms,
                      void *ctx)
{
    ProbeFake *fake = ctx;
    ++fake->calls;
    assert(timeout_ms == 5u);
    assert((address == 0x38u && (reg == 0xA3u || reg == 0xA8u) && !reg16) ||
           ((address == 0x5Du || address == 0x14u) && reg == 0x8140u && reg16));
    if (address != fake->hit_address) {
        return false;
    }
    if (address == 0x38u && reg == 0xA3u) {
        data[0] = fake->chip_id;
    } else if (address == 0x38u && reg == 0xA8u) {
        data[0] = fake->vendor_id;
    } else {
        memcpy(data, fake->gt_id, length);
    }
    return true;
}

static void test_health(void)
{
    DiagnosticsInputs in = {0};
    assert(diagnostics_health(&in) == HEALTH_BOOTING);
    in.init_complete = true;
    assert(diagnostics_health(&in) == HEALTH_OK);
    in.uart_overruns = 1u;
    assert(diagnostics_health(&in) == HEALTH_DEGRADED);
    in.uart_overruns = 0u;
    in.frame_misses = 2u;
    assert(diagnostics_health(&in) == HEALTH_OK);
    in.frame_misses = 3u;
    assert(diagnostics_health(&in) == HEALTH_DEGRADED);
    in.init_failed = true;
    assert(diagnostics_health(&in) == HEALTH_FAULT);
    /* MSP/link loss is deliberately absent from board-health inputs. */
}

static void test_watchdog_gate(void)
{
    DiagnosticsWatchdogGate gate;
    diagnostics_watchdog_gate_init(&gate);
    diagnostics_watchdog_progress(&gate, DIAG_PROGRESS_MSP);
    diagnostics_watchdog_progress(&gate, DIAG_PROGRESS_UI);
    assert(!diagnostics_watchdog_take_refresh(&gate));
    diagnostics_watchdog_progress(&gate, DIAG_PROGRESS_LED);
    assert(diagnostics_watchdog_take_refresh(&gate));
    assert(!diagnostics_watchdog_take_refresh(&gate));
}

static void test_watchdog_start_results(void)
{
    diagnostics_init();
    diagnostics_set_init_result(true);
    diagnostics_watchdog_apply_start_result(WATCHDOG_NOT_STARTED_FAIL);
    assert(diagnostics_get()->state == HEALTH_FAULT);
    assert(!diagnostics_watchdog_is_started());

    diagnostics_init();
    diagnostics_set_init_result(true);
    diagnostics_watchdog_apply_start_result(WATCHDOG_STARTED_OK);
    assert(diagnostics_get()->state == HEALTH_OK);
    assert(diagnostics_watchdog_is_started());

    diagnostics_init();
    diagnostics_set_init_result(true);
    diagnostics_watchdog_apply_start_result(WATCHDOG_STARTED_CONFIG_FAIL);
    assert(diagnostics_get()->state == HEALTH_FAULT);
    /* Once hardware has started, fault reporting must not disable feeding. */
    assert(diagnostics_watchdog_is_started());
}

static void test_ws2812_busy_drop_snapshot(void)
{
    diagnostics_init();
    diagnostics_set_runtime(30u, 100u, 2u, 0u, 5u, 42u, 7u);
    assert(diagnostics_get()->ws2812_busy_drops == 7u);
}

static void test_touch_probe(void)
{
    ProbeFake fake = {.hit_address = 0x38u, .chip_id = 0x54u, .vendor_id = 0x11u};
    assert(touch_probe_identify(fake_read, &fake) == TOUCH_FT_FAMILY);
    assert(fake.calls == 2u);
    fake = (ProbeFake){.hit_address = 0x38u, .chip_id = 0x55u, .vendor_id = 0x11u};
    assert(touch_probe_identify(fake_read, &fake) == TOUCH_NONE);
    assert(fake.calls == 4u);
    fake = (ProbeFake){.hit_address = 0x5Du, .gt_id = {'9','1','1','0'}};
    assert(touch_probe_identify(fake_read, &fake) == TOUCH_GT_FAMILY);
    assert(fake.calls == 2u);
    fake = (ProbeFake){0};
    assert(touch_probe_identify(fake_read, &fake) == TOUCH_NONE);
    assert(fake.calls == 3u);
}

void test_diagnostics(void)
{
    test_health();
    test_watchdog_gate();
    test_watchdog_start_results();
    test_ws2812_busy_drop_snapshot();
    test_touch_probe();
}
