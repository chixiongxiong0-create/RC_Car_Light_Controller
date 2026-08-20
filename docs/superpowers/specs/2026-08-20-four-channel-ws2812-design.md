# Four-Channel WS2812 Output Design

**Date:** 2026-08-20  
**Status:** Approved for implementation planning

## Goal

Replace the existing single SPI3/PB5 WS2812 chain with four independently
animated WS2812 outputs on the Wio Lite AI STM32H725 board:

| Group | Pixel count | Required behavior |
|---|---:|---|
| WS1 | 4 | independent frame and animation |
| WS2 | 4 | independent frame and animation |
| WS3 | 8 | independent frame and animation |
| WS4 | 8 | independent frame and animation |

The design must preserve the display, touch input, PSRAM, USART3 MSP link,
PF3/D10 front-lamp switch, PE10/D11 roof-spotlight switch, boot layout, and
the existing rule that demo UI state must never energize physical lights.

## Selected Architecture

Use two timer PWM engines with one DMA burst stream per timer:

- TIM2 drives the two four-pixel groups through CH1 and CH2.
- TIM3 drives the two eight-pixel groups through CH1 and CH2.
- Each timer receives an interleaved DMA buffer. One update request writes
  both channel compare registers for the next WS2812 bit.
- TIM2 and TIM3 may start a few timer clocks apart. Cross-group phase alignment
  is not a WS2812 requirement; every output remains internally valid.

This uses two timers and two DMA streams instead of four independent DMA
streams. Pairing equal-length groups on the same timer also gives both outputs
in a pair an identical frame length and reset interval.

## Pin Assignment

| Group | STM32 pin | Board header identity | Alternate function |
|---|---|---|---|
| WS1 | PA0 | Arduino D9 | AF1 TIM2_CH1 |
| WS2 | PB3 | Arduino D12 | AF1 TIM2_CH2 |
| WS3 | PB4 | Arduino MISO | AF2 TIM3_CH1 |
| WS4 | PB5 | Arduino MOSI | AF2 TIM3_CH2 |

The Wio Lite AI revision 1.0 schematic exposes all four pins on the Arduino
headers. The STM32H725 alternate-function table assigns PA0/PB3 to TIM2 and
PB4/PB5 to TIM3 as listed above.

PB5 stops being SPI3_MOSI. SPI3 initialization, its TX DMA allocation, and its
interrupt path are removed from the WS2812 runtime after the timer transport is
verified. PF3 and PE10 retain their existing high-power lamp GPIO roles.

## Timer and DMA Transport

Both timers generate an approximately 800 kHz PWM period. Exact prescaler,
auto-reload, and compare values are derived from the actual timer kernel clock
used by the firmware. For a 275 MHz timer clock, the starting values are:

| Parameter | Initial value | Result |
|---|---:|---|
| Prescaler | 0 | no input division |
| Auto-reload | 343 | about 799.4 kHz |
| logical 0 compare | about 96 | about 0.35 us high |
| logical 1 compare | about 193 | about 0.70 us high |

The final compare values must be measured at the 74AHCT125 output and adjusted
if clock-tree rounding makes either pulse fall outside the installed WS2812
variant's limits.

Each timer uses an update-triggered DMA burst beginning at CCR1 with a burst
length of two transfers. The buffer layout is:

```text
TIM2: WS1 bit 0 duty, WS2 bit 0 duty,
      WS1 bit 1 duty, WS2 bit 1 duty, ...

TIM3: WS3 bit 0 duty, WS4 bit 0 duty,
      WS3 bit 1 duty, WS4 bit 1 duty, ...
```

Every RGB pixel is serialized in GRB order, most-significant bit first. At
least 64 zero-duty periods are appended, producing about 80 us of reset-low
time. DMA buffers use a DMA-accessible RAM region, suitable alignment, and the
same explicit D-cache cleaning discipline already required by the STM32H725.

The intended initial DMAMUX allocation is one unused DMA1 stream for TIM2_UP
and one unused DMA1 stream for TIM3_UP. The exact stream numbers are finalized
in the implementation plan after checking every compiled peripheral path; the
request identities are fixed, but DMAMUX allows safe stream selection.

## Software Boundaries

The logical lighting policy and the physical waveform transport remain
separate:

1. `lighting_controller` consumes only real, freshness-checked MSP/RC vehicle
   state and produces four logical RGB frames plus the two 12 V lamp commands.
2. A four-output WS2812 encoder converts those frames into two interleaved PWM
   DMA buffers.
3. A timer/DMA platform port starts non-blocking transfers, tracks completion,
   and forces all four pins low when stopped or faulted.
4. The existing current limiter operates across the combined 24 installed
   pixels before encoding. A per-group rendering decision must not bypass the
   total current budget.

The application submits one coherent four-group frame. If either timer is
still busy, the transport rejects the whole new frame rather than updating
only one pair and producing a visually torn state. Animations may advance on
the next application tick.

## Animation and Pixel Mapping

All four groups have independent RGB storage and may display different effects
at the same time. The initial logical mapping is:

- WS1 and WS2: the two four-pixel vehicle lamp clusters. Existing running,
  brake, reverse, turn, and post-link-loss warning behavior is preserved and
  assigned to the appropriate physical side during hardware acceptance.
- WS3 and WS4: the two eight-pixel decorative strips. Each receives an
  independent frame; a shared mode selector may still choose the effect while
  phase, direction, color, or position can differ per group.

The physical left/right identity is deliberately verified on the bench before
permanent installation. Firmware group numbering must not be silently swapped
to compensate for crossed harness wiring; the accepted mapping is documented.

## Electrical Interface

Use one 74AHCT125 powered from the protected LED 5 V rail:

```text
PA0 -- 100 ohm -- 1A  74AHCT125 1Y -- 330 ohm -- WS1 DIN
PB3 -- 100 ohm -- 2A  74AHCT125 2Y -- 330 ohm -- WS2 DIN
PB4 -- 100 ohm -- 3A  74AHCT125 3Y -- 330 ohm -- WS3 DIN
PB5 -- 100 ohm -- 4A  74AHCT125 4Y -- 330 ohm -- WS4 DIN
```

- Tie 1OE through 4OE low so all channels are enabled.
- Place 100 nF directly at the 74AHCT125 VCC/GND pins.
- Power the 24 pixels from a separately fused 5 V, 3 A BEC.
- Join BEC, Wio, 74AHCT125, and all WS2812 grounds at a low-impedance common
  ground.
- Retain the protected 5 V input bulk capacitor and add about 100 uF near each
  remote group connector when harness length warrants it.
- Do not connect a 5 V logic output back to an STM32 pin.

At the legacy 60 mA-per-pixel worst-case estimate, 24 full-white pixels could
request 1.44 A. The 3 A BEC provides wiring and transient margin; the existing
1 A software current budget remains the default and is not a substitute for a
fuse or correctly sized conductors.

## Startup, Busy, and Failure Behavior

- Configure all four pins low before selecting their timer alternate functions.
- Before the first valid full MSP_RC lighting frame, transmit black only.
- On stale, malformed, or lost lighting RC, preserve the approved fail-safe
  policy: high-power lamps and decorative strips off; the two four-pixel rear
  groups may show only the low-current amber link-loss warning after a link was
  previously valid.
- A DMA or timer start failure immediately disables both timer outputs and
  leaves all four pins low. It must not fall back to blocking bit-banging.
- A transport busy condition drops the candidate physical frame and records a
  diagnostic; it does not partially update one timer pair.
- Transfer-complete handling stops DMA requests, forces a zero compare value,
  and leaves each output low through the reset interval.

## Migration Strategy

1. Add a host-testable four-frame-to-interleaved-PWM encoder while preserving
   the existing SPI encoder and application wiring.
2. Add the TIM2/TIM3 DMA-burst port and verify its initialization and failure
   behavior with host manifests and focused unit tests.
3. Change the lighting service to submit four groups atomically through the new
   port.
4. Remove the SPI3 WS2812 runtime only after the timer path builds and its host
   tests pass. Do not remove unrelated SPI support that another compiled board
   feature still uses.
5. Update hardware wiring and acceptance documentation from one chain to four
   connectors.

## Verification

Firmware verification requires:

- focused encoder tests covering GRB order, logical 0/1 duty, four independent
  frames, both group lengths, reset slots, bounds, and guard bytes;
- transport tests or source manifests covering exact pins, alternate functions,
  timer channels, DMA requests, safe-low initialization, atomic busy behavior,
  completion, and error shutdown;
- lighting policy regression tests, especially real-state-only drive, current
  limiting, startup safety, short MSP_RC invalidation, independent RC timeout,
  and post-valid link-loss indication;
- a clean full host CTest run;
- a fresh ARM ELF/HEX/BIN build, application vector at `0x08020000`, and no
  unresolved symbols;
- a diff audit proving no unintended display, touch, PSRAM, USART3, boot-stub,
  PF3, or PE10 changes.

Physical acceptance remains pending until hardware is connected. Using a logic
analyzer at each 74AHCT125 output, verify frequency, T0H, T1H, reset-low time,
all four independent patterns, correct group identity, power-rail droop, maximum
current, reset behavior, and link-loss behavior before permanent installation.

## Explicit Non-Goals

- No blocking cycle-counted GPIO waveform generator.
- No external lighting coprocessor for the current 24-pixel installation.
- No exact cross-timer phase synchronization requirement.
- No increase of the existing default 1 A software current budget.
- No change to the two 12 V lamp power stages or their PF3/PE10 controls.
