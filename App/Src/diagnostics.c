#include "diagnostics.h"

#include <string.h>

#include "platform/msp_uart.h"

static DiagnosticsSnapshot snapshot;

void diagnostics_init(void)
{
    memset(&snapshot, 0, sizeof snapshot);
    snapshot.state = HEALTH_BOOTING;
}

void diagnostics_tick(uint32_t now_ms)
{
    (void)now_ms;
    snapshot.uart_overruns = msp_uart_overruns();
}

const DiagnosticsSnapshot *diagnostics_get(void)
{
    return &snapshot;
}
