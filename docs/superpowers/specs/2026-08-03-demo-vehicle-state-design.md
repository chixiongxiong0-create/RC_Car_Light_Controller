# Demo Vehicle State Design

## Goal

Make the UI visibly animate when the board has never received valid MSP data,
without weakening real-vehicle link-loss behavior or mixing synthetic frames
with the MSP parser.

## User-visible behavior

- On boot, before the first valid MSP frame, the UI consumes a synthetic
  `VehicleState` and displays a visible `DEMO` indicator.
- Demo values animate the dashboard, mechanical face, and LED effects.
- The demo does not change pages. `aux_page` remains in the neutral zone, so
  the `USER1` short press continues to cycle dashboard, face, and showcase.
- The first valid MSP frame permanently disables demo mode for that boot.
- If MSP later becomes stale or lost, the UI retains real-vehicle semantics
  and shows `LINK STALE` or `LINK LOST`; it never falls back to demo data.

## Architecture

Add a standalone `demo_vehicle_state` module. It owns only demo generation
and has no access to the UART transport or MSP parser. The application chooses
between the demo state and the real `vehicle_state_get()` result before passing
one read-only `VehicleState` pointer to input, UI, low-battery, diagnostics, and
LED consumers.

The application records a `real_msp_seen` latch when
`vehicle_state_on_msp()` accepts its first valid frame. The latch is cleared
only by reboot. Until it is set, the selected state is the demo state. Once it
is set, the selected state is always the real state, including during later
link loss.

Synthetic MSP packets must not be injected into `msp_client` or
`vehicle_state_on_msp()`. This preserves parser, timeout, and link-health
semantics.

## Demo data model

The generator is deterministic and driven by `now_ms`; it requires no random
number generator and remains reproducible in host tests.

- Throttle follows a slow cycle containing forward motion, a stop, and a short
  low-speed reverse segment.
- Steering sweeps smoothly left and right.
- Roll and pitch use slow bounded waves representing crawler body movement.
- Heading advances slowly and wraps cleanly at 360 degrees.
- Battery voltage stays safely above the configured low-battery threshold with
  a small bounded variation. If the battery cell count is unknown, use a
  conservative display-only nominal voltage and do not enable low-battery
  warnings.
- RSSI and GPS satellite count vary within plausible noncritical ranges.
- Mode flags and armed state use explicitly recognizable demo-safe values;
  demo must never present itself as a genuinely armed vehicle.
- `aux_page` remains `0.0f`, and demo link state is presentation-only rather
  than evidence of an MSP connection.

All generated values must remain within the ranges already accepted by the
UI models: normalized control inputs in `[-1, 1]`, bounded crawler attitude,
heading in `[0, 360)`, and valid integer ranges for RSSI and satellites.

## Real-data handoff

On the first accepted MSP frame, begin a 300 ms presentation blend from the
last demo sample toward the evolving real state. Blend continuous numeric
fields only. Discrete safety and link fields (`link`, `armed`, mode flags, GPS
satellite count) switch immediately to the real values so the transition
cannot conceal vehicle status.

The blend belongs in a small state-selection layer, not in the MSP parser.
After 300 ms, consumers receive the real state directly. No later timeout may
restart the blend or demo generator.

## Status indication

Expose whether the selected state is demo data to the UI. The persistent top
status area displays `DEMO` using a distinct non-alarm color. `DEMO` disappears
immediately when the first valid MSP frame is accepted, even while continuous
values finish their short blend.

The demo flag is presentation metadata and must not overwrite `VehicleState`
link health or diagnostics counters.

## Failure handling and safety

- Invalid or unsupported MSP frames do not disable demo mode.
- A valid MSP frame disables demo mode even if it supplies only one supported
  data group; subsequent polling fills the remaining fields normally.
- MSP stale/lost handling remains unchanged after real-data takeover.
- Demo data never drives the real vehicle, writes to INAV, or transmits MSP
  commands beyond the existing polling behavior.
- Demo mode must not become authoritative for AUX page selection.

## Verification

Host tests cover:

- deterministic output for a given timestamp;
- bounds and representative motion of every animated field;
- neutral demo AUX and unarmed demo status;
- invalid MSP input leaving demo enabled;
- first valid MSP frame latching real-data ownership;
- immediate switching of discrete safety fields;
- completion of the numeric blend after 300 ms;
- later `LINK_STALE` and `LINK_LOST` states never returning to demo;
- button page cycling remaining available while demo is active.

Hardware acceptance checks that the three pages visibly animate without an
INAV connection, `USER1` still changes pages, `DEMO` is visible, and connecting
INAV removes `DEMO` and transfers control without a visible numeric jump.
