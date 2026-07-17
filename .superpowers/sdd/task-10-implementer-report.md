# Task 10 Implementer Report

## Outcome

Implemented the current-limited WS2812 decoration controller and a non-blocking
SPI3 TX-DMA port on PB5.

- Policy priority is board fault, lost link, low battery, turn/reverse, page.
- Left and right indications split the configured strip into two segments.
- Low battery uses a three-pulse sequence in a 5 second period; lost link uses
  a double-pulse sequence.
- Pixel count is compile-time constrained to 10 through 30, with 30 as default.
- The current estimator uses 20 mA per channel at 255 and integer scaling to a
  1000 mA default budget.
- The shared low-battery policy is evaluated once in `app.c`; the same result is
  consumed by the UI face and the LED controller.
- SPI3 uses PB5 AF6 and DMA1 Stream1. The 384-byte aligned D1 SRAM buffer is
  cleaned from D-cache before DMA. Submissions are non-blocking and limited to
  at most one every 34 ms. Only the `hspi3` completion callback releases busy.

## Clock-tree Decision

The approved 2.4 MHz/3-bit proposal cannot be generated from an existing SPI123
source with an integer SPI prescaler without changing a shared clock:

- PLL1Q is 110 MHz;
- PLL2P is 133 MHz while PLL2 also supplies OSPI;
- PLL3P is 120 MHz while PLL3 also supplies LTDC;
- CLKP is 64 MHz.

Per the user's selected option A, LTDC, OSPI, and all PLL parameters remain
unchanged. SPI3 uses the existing PLL1Q source at 110 MHz divided by 32, or
3.4375 MHz. Each WS2812 bit is encoded with four SPI bits (`0=1000`, `1=1110`)
in GRB order. Thirty pixels require 360 encoded bytes plus 24 zero reset bytes.

Theoretical values are about 1.164 us per WS2812 cell, 0.291 us T0H, and
0.873 us T1H. These are design calculations, not measured electrical results.
A logic analyzer and the actual LED strip must verify compatibility.

## TDD Evidence

RED was observed before production files existed: CMake failed because
`App/Src/led/led_controller.c` was missing.

GREEN covers policy priority, page/reverse/turn colors, left/right segments,
flash phases, 1000 mA limiting, count overflow clamping, exact GRB bit patterns,
reset length, 10/30-pixel encoded lengths, buffer guards, DMA busy and 30 Hz
scheduling conditions, and tick wrap-around.

## Build and Size

The first CMake Debug build used 385640 Flash bytes and missed the required
20 KiB headroom. App-only `-Os` improved it to 379496 bytes but remained below
the gate. Debug symbols (`-g3`) were retained, and size optimization was scoped
to firmware C sources (App, Core and Drivers); LVGL retains its existing source
policy. CMake was also brought in line with Make's existing function/data
sections.

Final sizes:

```text
CMake Debug: text=345992 data=1864 bss=582240
Flash=347856 bytes; 384 KiB headroom=45360 bytes

Make Debug: text=332148 data=568 bss=581760
Flash=332716 bytes; bin=332724 bytes
```

## Deferred Hardware Verification

Before claiming hardware success:

1. Drive PB5 through a 3.3 V to 5 V level shifter and a 220-470 ohm series
   resistor, with a common ground and an appropriately fused LED supply.
2. Measure SPI clock, T0H, T1H, total cell time and reset-low duration.
3. Exercise both 10- and 30-pixel configurations at the 1 A software budget.
4. Confirm colors, no board reset, and no visible UI frame-rate regression.

No electrical timing, current draw, or target-board behavior is claimed by this
host/build-only task.
