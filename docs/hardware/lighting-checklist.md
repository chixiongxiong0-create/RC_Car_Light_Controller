# Crawler lighting hardware checklist

Firmware lighting policy is complete, but physical bench acceptance is **pending** until the vehicle wiring and behavior below are confirmed. Do not treat host tests or build output as proof that a lamp, MOSFET, or LED strip is safe on the real vehicle.

## Default MSP channel map

The controller requires a complete 13-channel (26-byte) `MSP_RC` frame. Under the default AETR-plus-AUX ordering, the zero-based MSP channel indices are shown below.

| MSP channel | Function | Default behavior |
| --- | --- | --- |
| 9 (AUX6) | front 12 V lamps | brightness/duty request |
| 10 (AUX7) | roof 12 V spotlights | brightness/duty request |
| 11 (AUX8) | roof WS2812 mode | selects one of eight stable mode bands |
| 12 (AUX9) | effect parameter | continuous brightness or animation-speed parameter |

An RC payload shorter than 26 bytes cannot carry AUX9 and is not valid for lighting control. On startup, a malformed or stale frame, link loss, or demo-only state, PF3 and PE10 remain low, roof pixels are off, and lamps must remain off. Never use demo UI state to energize lights.

## 12 V lamp wiring

Use two external, low-side **BSZ028N04LS** MOSFET switches. Both GPIOs are active high:

| Lamp | Wio signal | Gate network |
| --- | --- | --- |
| front | PF3 / Arduino D10 | GPIO -> 47-100 ohm series resistor -> MOSFET gate; 10 kohm gate-to-ground pulldown |
| roof spotlights | PE10 / Arduino D11 | GPIO -> 47-100 ohm series resistor -> MOSFET gate; 10 kohm gate-to-ground pulldown |

For each lamp circuit: `battery positive -> fuse -> lamp positive`; `lamp negative -> MOSFET drain`; `MOSFET source -> battery negative`. Fit the planned bulk capacitor and TVS protection on the lamp supply, and connect Wio, LED supply, INAV, and battery grounds together. Never route lamp current through the Wio board or its ground traces.

GPIOs are initialized low before being made outputs. The current firmware treats these as switched outputs (the policy carries a 0..1000 request, while the GPIO port uses a 50% threshold); add a suitable PWM allocation only if dimming is required.

## WS2812 four-channel wiring

There are four independent pixel outputs, not one continuous chain. The connector order is fixed:

```text
PA0/D9   -> 100R -> 74AHCT125 1A/1Y -> 330R -> WS1 DIN (4 pixels)
PB3/D12  -> 100R -> 74AHCT125 2A/2Y -> 330R -> WS2 DIN (4 pixels)
PF11/A1  -> 100R -> 74AHCT125 3A/3Y -> 330R -> WS3 DIN (8 pixels)
PF12/A3  -> 100R -> 74AHCT125 4A/4Y -> 330R -> WS4 DIN (8 pixels)
```

The group order is firmware groups 0..3 and is fixed at 4/4/8/8 pixels (24 total).
Do not connect a DOUT from one group to the DIN of another. Power the 74AHCT125 from
protected 5 V, tie **all four active-low OE pins low**, and fit a **100 nF** local
decoupling capacitor at its VCC/GND pins. The 100 ohm resistors are MCU-to-AHCT input
series resistors; the 330 ohm resistors are AHCT-output-to-DIN series resistors.

Power pixels from a fused 5 V / 3 A BEC and share its ground with Wio, INAV, and the
vehicle. Put protected input bulk capacitance on the LED branch; an optional 100 uF
capacitor may be fitted at each remote group. The 1 A firmware current budget covers
all 24 pixels, but does not replace BEC, fuse, wire, capacitor, TVS, or current-limit
protection. No 5 V source, AHCT output, or WS2812 DOUT may feed back into PA0, PB3,
PF11, or PF12.

## Light behavior and controls

Rear pixels run dim red with a healthy real MSP link. A forward-to-neutral/reverse deceleration infers braking and holds all four bright red for at least 600 ms. After that hold, reverse makes the inner pair white while the outer pair retain running red. Turn input below/above the left/right threshold blinks the selected pair at a 666 ms period (333 ms on); in reverse, the inner reverse pixel remains white while its outer pixel can blink amber. Board-fault, low-battery, and link-loss safety indications take priority over decorative roof output; a previously healthy lost link uses a low-current amber double flash when frames continue to be rendered.

The AUX8 mode order is:

1. off
2. steady white
3. warm trail light
4. breathing amber
5. moving comet
6. rainbow chase
7. police red/blue demonstration
8. battery/status visualization

AUX8 has hysteresis between mode bands. AUX9 is continuous: it controls brightness in static modes and animation speed (with a conservative brightness ceiling) in animated modes.

## Final vehicle confirmation record

| Confirm on the real vehicle | Default / assumption | If different |
| --- | --- | --- |
| Header identities | PF3 is D10; PE10 is D11 | update the GPIO port pin definitions |
| Output polarity | high turns each MOSFET on | invert the output-port polarity after bench proof |
| RC ordering | AETR followed by AUX1; AUX6..AUX9 are indices 9..12 | correct the MSP channel mapping |
| Throttle polarity | negative is reverse | correct throttle interpretation |
| Steering polarity | negative is left | correct steering interpretation |
| WS1 physical identity/order | group 0; 4 pixels | PENDING — record the installed left/right identity and correct the logical mapping if needed |
| WS2 physical identity/order | group 1; 4 pixels | PENDING — record the installed left/right identity and correct the logical mapping if needed |
| WS3 physical identity/order | group 2; 8 pixels | PENDING — record the installed location and DIN direction |
| WS4 physical identity/order | group 3; 8 pixels | PENDING — record the installed location and DIN direction |
| Total pixels | WS1/WS2/WS3/WS4 = 4/4/8/8 (24 total) | PENDING — correct hardware before any permanent installation |
| Current budget | BEC, fuse, wire, capacitor, TVS support installed load | revise hardware sizing and brightness budget |
| Brake feel | inferred deceleration and 600 ms hold feel natural | tune brake threshold/hold constants |
| Link-loss behavior | lamps off; rear warning only after a real link existed | verify and adjust only after safe bench observation |

## Safe bench sequence

1. Disconnect the 12 V lamps. Check continuity, fuse placement, MOSFET orientation, and polarity with power removed.
2. Use current-limited supplies. Verify the 5 V BEC before connecting Wio or pixels, and verify the lamp supply separately.
3. Reset the Wio and measure PF3 and PE10 low before and after GPIO initialization.
4. Connect and test one MOSFET output with a safe test load, then disconnect it again.
5. Connect WS1 only, then WS2, WS3, and WS4 one at a time. Verify each DIN identity,
   left/right identity where applicable, color/order, reset low, and safe-low behavior; record every result as PENDING until it is measured.
6. Add all four groups, confirm all 74AHCT125 OE pins are low, the 100 nF decoupling,
   protected input bulk capacitance, optional remote 100 uF capacitors if fitted, total 4/4/8/8 count, and 1 A current budget.
7. With lamps still managed by a current-limited setup, verify AUX6-AUX9 mapping and all eight AUX8 ranges.
8. Test stale/link-loss behavior and confirm both 12 V outputs turn off.
9. Only after these checks pass may the vehicle battery and permanent lamps be connected.

## Blank-board flashing reminder

Keep the existing two-image boot layout: a blank board (or one without the confirmed stable sector-0 stub) needs both `boot_stub/wio_ai_boot_stub.hex` at `0x08000000` and the application `build_ws4_final/wio_ai.hex` at `0x08020000`. **No flash was performed for this four-channel documentation or software-verification task.**
