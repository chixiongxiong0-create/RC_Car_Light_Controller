# RC Crawler Lighting Controller Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add fail-safe MSP-controlled front, roof, rear, and decorative roof lighting to the existing STM32H725 Wio Lite AI firmware.

**Architecture:** Extend the immutable real `VehicleState` with AUX6-AUX9. A pure `lighting_controller` owns channel interpretation, brake inference, rear safety patterns, and roof effects; a small hardware port owns the two MOSFET GPIOs while the existing SPI3/DMA WS2812 port transmits one combined rear-plus-roof frame.

**Tech Stack:** C11, STM32H725 HAL, INAV MSP v1 `MSP_RC`, SPI3 DMA, WS2812B, CMake/CTest host tests, GNU Arm Embedded 13.3.

## Global Constraints

- Preserve the original source at `E:/workspace/stm32/wio_lite_ai/wio_lite_ai`.
- Preserve the working display, PF5 backlight, USART3 MSP transport, SPI3/PB5 waveform, boot stub, and application vector at `0x08020000`.
- Never energize physical lights from demo vehicle data.
- Never send `MSP_SET_RAW_RC` or mutate INAV configuration.
- Default rear order is left outer, left inner, right inner, right outer.
- Default outputs are PF3/D10 for front lamps and PE10/D11 for roof spotlights, active high.
- AUX6-AUX9 are MSP zero-based channel indices 9-12.
- Keep all policy host-testable and all output operations non-blocking.

---

## File Structure

- `App/Inc/vehicle_state.h`, `App/Src/vehicle_state.c`: validated real MSP channel data.
- `App/Inc/lighting/lighting_controller.h`, `App/Src/lighting/lighting_controller.c`: pure lighting state machine and renderer.
- `App/Inc/platform/lighting_output_port.h`, `App/Src/platform/lighting_output_port.c`: safe PF3/PE10 initialization and writes.
- `App/Src/app.c`: connect real state, lighting policy, GPIO port, current limiter, and WS2812 transport.
- `App/Inc/app_config.h`: pixel segmentation and adjustable default constants.
- `tests/test_vehicle_state.c`, `tests/test_lighting_controller.c`: deterministic policy verification.
- `tests/CMakeLists.txt`, `tests/test_main.c`, `Makefile`, `cmake/st-project.cmake`: build integration.
- `docs/hardware/lighting-checklist.md`: final physical verification and tuning record.

### Task 1: Decode AUX6-AUX9 Without Breaking Existing MSP State

**Files:**
- Modify: `App/Inc/vehicle_state.h`
- Modify: `App/Src/vehicle_state.c`
- Modify: `tests/test_vehicle_state.c`

**Interfaces:**
- Produces: `float aux6`, `aux7`, `aux8`, `aux9` in `VehicleState`.
- Preserves: `bool vehicle_state_on_msp(const MspFrame *, uint32_t)` and existing channel behavior.

- [ ] Add a test frame containing 13 little-endian channels and assert indices 9-12 normalize from 1000/1250/1750/2000 us to -1.0/-0.5/+0.5/+1.0.
- [ ] Add a short ten-byte RC frame test and assert existing steering/throttle/page parsing still succeeds while AUX6-AUX9 remain safe at -1.0.
- [ ] Run `cmake -S tests -B build-host-lighting && cmake --build build-host-lighting && ctest --test-dir build-host-lighting --output-on-failure`; confirm the new assertions fail first.
- [ ] Initialize AUX6-AUX9 to -1.0 and decode them only when the payload has at least 26 bytes; clamp pulse inputs to 1000..2000 before normalization.
- [ ] Re-run the host suite and commit only Task 1 files with `feat: decode MSP lighting channels`.

### Task 2: Implement the Pure Lighting State Machine

**Files:**
- Create: `App/Inc/lighting/lighting_controller.h`
- Create: `App/Src/lighting/lighting_controller.c`
- Create: `tests/test_lighting_controller.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tests/test_main.c`

**Interfaces:**
- Produces: `void lighting_controller_init(LightingController *)`.
- Produces: `void lighting_controller_render(LightingController *, uint32_t now_ms, const VehicleState *, bool low_battery, bool board_fault, LightingFrame *, size_t pixel_count)`.
- `LightingFrame` contains `uint16_t front_duty`, `uint16_t roof_spot_duty`, `RoofLightMode roof_mode`, and `LedRgb pixels[LED_MAX_PIXELS]`.

- [ ] Write failing tests for startup-off, AUX6/AUX7 mapping, eight AUX8 mode bands, AUX9 continuity, minimum/maximum pixel counts, and invalid arguments.
- [ ] Write failing tests for dim-red running lights; 600 ms brake hold after forward deceleration; reverse white inner pixels; left/right amber flashes; and combined reverse-turn behavior.
- [ ] Write failing tests for stale/lost link, low battery, and board fault priorities.
- [ ] Run the new focused test executable and confirm failures are caused by missing policy code.
- [ ] Implement normalized-to-unit conversion, mode-band hysteresis, brake history/hold state, rear overlays, roof effects, and bounded frame rendering without hardware calls.
- [ ] Run all host tests and commit Task 2 files with `feat: add crawler lighting policy`.

### Task 3: Add Safe 12 V Lamp GPIO Output Port

**Files:**
- Create: `App/Inc/platform/lighting_output_port.h`
- Create: `App/Src/platform/lighting_output_port.c`
- Modify: `App/Inc/app_config.h`
- Modify: `Makefile`
- Modify: `cmake/st-project.cmake`

**Interfaces:**
- Produces: `void lighting_output_port_init(void)`.
- Produces: `void lighting_output_port_apply(uint16_t front_duty, uint16_t roof_duty)`.
- Consumes: duty range 0..1000; values below 500 are off and values at least 500 are on.

- [ ] Add `APP_REAR_PIXEL_COUNT=4`, GPIO port/pin defaults, and compile-time pixel-count validation to `app_config.h`.
- [ ] Implement initialization that enables GPIOF/GPIOE clocks, writes PF3 and PE10 low before configuring push-pull outputs, and uses low speed/no pull.
- [ ] Implement clamped threshold writes using `HAL_GPIO_WritePin` and document that hardware PWM is intentionally deferred.
- [ ] Add the source to both ARM build systems and compile it with the firmware target.
- [ ] Commit Task 3 files with `feat: add fail-safe lamp GPIO outputs`.

### Task 4: Integrate Real MSP State and Combined WS2812 Frame

**Files:**
- Modify: `App/Src/app.c`
- Modify: `App/Src/led/led_controller.c`
- Modify: `App/Inc/led/led_controller.h`
- Modify: `tests/test_led_controller.c`

**Interfaces:**
- Consumes: `LightingController`, `LightingFrame`, real `VehicleState`, and existing `ws2812_port_submit`.
- Produces: one current-limited chain whose first four pixels are rear safety lamps and remaining pixels are the roof strip.

- [ ] Replace the application call to the legacy page-colored LED renderer with `lighting_controller_render` using `real_state`, never `presented` demo state.
- [ ] Initialize and apply the GPIO output port; apply off outputs before MSP is healthy.
- [ ] Keep `led_estimated_ma` and `led_limit_current` as reusable utilities, removing only obsolete policy code and updating its tests accordingly.
- [ ] Run the complete host suite and compile the ARM firmware.
- [ ] Commit Task 4 files with `feat: integrate MSP lighting outputs`.

### Task 5: Hardware Documentation and Final Verification

**Files:**
- Create: `docs/hardware/lighting-checklist.md`
- Modify: `README.md`

**Interfaces:**
- Documents: channel mapping, physical wiring, polarity, power protection, defaults, tuning constants, and bench-test sequence.

- [ ] Record the one-chain WS2812 order, PF3/PE10 MOSFET wiring, 74AHCT level shifter, separate 5 V BEC, fuse, capacitor, TVS, and common-ground requirements.
- [ ] Record all assumptions that need final user confirmation and exact source constants to change if polarity/order differs.
- [ ] Configure and run a fresh host build and require every CTest test to pass.
- [ ] Run a fresh short-path ARM build and require ELF, HEX, and BIN output.
- [ ] Verify `.isr_vector` VMA is `0x08020000`, run `git diff --check`, and inspect the final worktree without touching unrelated pre-existing changes.
- [ ] Commit documentation with `docs: add crawler lighting hardware checklist`.
