#include <assert.h>
#include <string.h>

#include "diagnostics.h"
#include "platform/touch_probe.h"

typedef struct {
    unsigned calls;
    uint8_t hit_address;
    uint8_t id[4];
} ProbeFake;

static bool fake_read(uint8_t address, uint16_t reg, bool reg16,
                      uint8_t *data, uint16_t length, uint32_t timeout_ms,
                      void *ctx)
{
    ProbeFake *fake = ctx;
    ++fake->calls;
    assert(timeout_ms == 5u);
    assert((address == 0x38u && reg == 0xA8u && !reg16) ||
           ((address == 0x5Du || address == 0x14u) && reg == 0x8140u && reg16));
    if (address != fake->hit_address) {
        return false;
    }
    memcpy(data, fake->id, length);
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

static void test_touch_probe(void)
{
    ProbeFake fake = {.hit_address = 0x38u, .id = {0x54u}};
    assert(touch_probe_identify(fake_read, &fake) == TOUCH_FT_FAMILY);
    assert(fake.calls == 1u);
    fake = (ProbeFake){.hit_address = 0x5Du, .id = {'9','1','1','0'}};
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
    test_touch_probe();
}
