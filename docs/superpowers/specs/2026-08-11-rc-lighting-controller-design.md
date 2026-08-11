# RC Crawler Lighting Controller Design

## Goal

Extend the existing Wio Lite AI UI firmware into a fail-safe lighting controller. The board reads INAV `MSP_RC` over the already dedicated USART3 link, drives two external 12 V LED power switches, and renders a four-pixel rear lamp cluster plus a roof WS2812B strip through the already verified SPI3/DMA transport.

The original Wio Lite AI source tree remains untouched. This work stays in the isolated `codex/rc-crawler-ui` worktree and does not change the working display, backlight, boot stub, application offset, or MSP UART wiring.

## Fixed Default Wiring and Channel Map

These defaults are deliberately centralized and must be checked on the finished vehicle:

| Function | Default |
|---|---|
| WS2812 physical chain | PB5/SPI3 -> level shifter -> rear pixels 0..3 -> roof pixels 4..N-1 |
| Rear order from DIN | left outer, left inner, right inner, right outer |
| Front 12 V lamp MOSFET gate | Arduino D10 / PF3 |
| Roof 12 V spotlight MOSFET gate | Arduino D11 / PE10 |
| AUX6 / MSP channel index 9 | front lamp brightness |
| AUX7 / MSP channel index 10 | roof spotlight brightness |
| AUX8 / MSP channel index 11 | roof WS2812 mode selector |
| AUX9 / MSP channel index 12 | effect parameter |

Both 12 V outputs are active-high 3.3 V logic signals into external low-side MOSFET gate networks. Each network requires a 47-100 ohm series gate resistor and a 10 kohm gate-to-ground resistor. GPIOs initialize low before becoming outputs.

## Architecture

`vehicle_state` validates and decodes at least 13 MSP RC channels. It retains steering and throttle and adds normalized AUX6 through AUX9 values without changing the existing UI-facing fields.

`lighting_controller` is pure, hardware-independent policy. Given the real vehicle state and time, it produces:

- front and roof power duty values from 0 to 1000;
- four rear WS2812 colors;
- the roof strip colors for the configured pixel count;
- a diagnostic mode identifier.

`lighting_output_port` owns PF3 and PE10 and initially implements safe on/off drive. Its API accepts 0..1000 duty values so hardware PWM can be added later without changing policy. The default implementation applies a 50% threshold because the current GPIO configuration has no dedicated PWM timer allocation. Continuous AUX input is preserved in policy and diagnostics, but the two 12 V outputs act as switches in this revision.

The existing `ws2812_port` remains a single SPI3/DMA transport. Logical rear and roof regions are rendered into one contiguous frame, so no additional timer, SPI, DMA stream, or interrupt is consumed.

Physical lighting always consumes the real MSP state. The UI demo state never energizes real lamps.

## Rear Lamp Policy

The rear cluster defaults to dim red running lights whenever MSP RC is healthy.

- Brake: when forward throttle was established and throttle falls rapidly toward neutral or reverse, all four pixels become bright red for at least 600 ms. This is an inference from RC commands, not a measurement of vehicle speed.
- Reverse: after the brake hold has elapsed, negative throttle lights the two inner pixels white. The outer pixels retain the dim red running light.
- Left turn: steering below -0.30 flashes the two left pixels amber at 1.5 Hz.
- Right turn: steering above +0.30 flashes the two right pixels amber at 1.5 Hz.
- Combined reverse and turn: the inner reverse pixel stays white and the outer turn pixel flashes amber on the selected side.
- Link stale or lost: both 12 V outputs turn off immediately. The rear cluster changes to a low-current amber double flash when output data is still being generated.
- Board fault and low battery retain higher priority than decorative roof effects.

Thresholds, polarity, rear order, blink timing, and brake hold time are compile-time defaults listed in the hardware checklist for final adjustment.

## Roof Strip Modes

AUX8 is divided into stable ranges with hysteresis so a noisy analog knob does not chatter between modes:

1. off;
2. steady white;
3. warm trail light;
4. breathing amber;
5. moving comet;
6. rainbow chase;
7. police red/blue demonstration;
8. battery/status visualization.

AUX9 is a continuous 0..1 effect parameter. For static modes it controls brightness. For animated modes it controls animation speed while retaining a conservative brightness ceiling. Every frame passes through the existing 1 A software current limiter.

Safety, brake, reverse, turn, low-battery, link-loss, and board-fault indications override decorative colors only in their assigned rear region. Roof decorative modes never override rear safety indications.

## Data Validity and Failure Behavior

An `MSP_RC` payload shorter than 26 bytes cannot provide AUX9 and is rejected for lighting control, while existing telemetry parsing remains backward-compatible. Channel values are clamped to 1000..2000 microseconds before normalization.

The lighting controller requires a healthy, recent real RC frame. On startup, stale data, link loss, malformed frames, or demo-only operation:

- PF3 and PE10 are low;
- roof WS2812 pixels are off;
- rear pixels are off during startup and use the link-loss indication only after a real link has previously existed.

No MSP write command, `MSP_SET_RAW_RC`, or INAV configuration mutation is used.

## Testing

Host tests cover:

- MSP channel index decoding and short-payload rejection;
- AUX6/AUX7 threshold mapping;
- mode selection and hysteresis;
- steady and animated roof modes;
- brake entry, minimum hold, release, and forward-to-reverse transition;
- left/right and combined reverse-turn rear patterns;
- startup and link-loss fail-safe behavior;
- rear/roof bounds for a 4-pixel minimum and 30-pixel maximum chain;
- current limiting and existing WS2812 encoding behavior.

Firmware verification requires a clean host CTest run, a fresh ARM build, `.isr_vector` at `0x08020000`, and no unresolved symbols. Hardware acceptance remains explicit: confirm PF3/PE10 header identity, MOSFET polarity, channel order, steering/throttle polarity, rear pixel order, strip length, visual modes, and link-loss behavior before permanent installation.

## Final Hardware Checkpoints

1. Rear DIN order really is left outer, left inner, right inner, right outer.
2. PF3 is Arduino D10 and PE10 is Arduino D11 on the actual board revision.
3. A high GPIO level turns each MOSFET on; external 10 kohm pulldowns keep both off during reset.
4. INAV MSP channel order matches AETR followed by AUX1, making AUX6..AUX9 indices 9..12.
5. Negative normalized steering means left and negative throttle means reverse.
6. Total WS2812 count equals `APP_LED_PIXEL_COUNT`, with at least four pixels.
7. The 5 V LED BEC, level shifter, fuse, capacitor, TVS, and common ground are installed.
8. Brake inference feels natural on the real ESC; adjust deceleration threshold and 600 ms hold if required.
