# STM32F469I-DISCO UI and Lighting Port Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a separate STM32F469I-DISCO firmware target with two native 800×480 touch interfaces while preserving the Wio UI board's MSP and lighting behavior.

**Architecture:** Keep MSP decoding, vehicle state, lighting policy, and WS2812 frame encoding as portable application code. Add a small F469 board support layer for display, touch, UART, timer/DMA, GPIO, watchdog, and startup. Replace the old three-screen navigation in the F469 target with a two-screen controller; keep the current Wio build working.

**Tech Stack:** C11, STM32CubeF4 HAL, STM32F469I-DISCO BSP, LVGL 9 already vendored in this repository, CMake/Ninja, host CTest, arm-none-eabi toolchain.

**Spec:** `docs/superpowers/specs/2026-09-24-stm32f469-disco-ui-port-design.md`

## Global Constraints

- INAV, ELRS, and the vehicle drive chain stay independent of the UI board.
- Use the board's native 800×480 landscape display and two main interfaces: Face and Information.
- Face manual selection expires 10 seconds after the last swipe; low battery and lost link override it.
- Information has Overview and Diagnostics subpages; horizontal swipes stay within the current main interface.
- Preserve AUX6 lamp switching, AUX8 LED mode, AUX9 LED parameter, and physical WS chains 4+4 and 8+8.
- Demo state must never energize lamps or LEDs.
- No 12 V on the board's 5 V net; lamp and LED current must not flow through the Discovery board.
- The repository currently has unrelated uncommitted edits. Never reset or overwrite them; stage only files owned by each task.
- Bench-only status until real board, light, power, and INAV tests pass.

## Review Focus

- A link loss during manual Face selection must show the warning immediately; add a controller test in Task 2.
- Return from a warning after the 10-second deadline must resume automatic selection; add a controller test in Task 2.
- Stale or invalid MSP values must show unknown on Overview; add a view-model test in Task 3.
- A button press while touch is unavailable must change only the main interface; add an input test in Task 2.
- A failed or busy LED DMA transfer must leave outputs low and allow a later frame; add a transport test in Task 6.

## File Structure

- `boards/stm32f469i_disco/`: F469 startup, linker script, clock, board drivers, HAL/BSP configuration, pin map, and its own CMake target. No H7 HAL files enter this target.
- `App/Inc/ui/two_screen_controller.h` and `App/Src/ui/two_screen_controller.c`: pure state machine for main interface, subpage, manual Face selection, and warning priority.
- `App/Inc/ui/overview_model.h` and `App/Src/ui/overview_model.c`: pure formatting/validity model for Overview.
- `App/Src/ui/f469_ui.c` and matching header: F469 LVGL objects for the two interfaces, using the pure models; old Wio UI files remain available to its build.
- `boards/stm32f469i_disco/ports/`: one hardware responsibility per file: display/touch, MSP UART, WS2812, lamp outputs, watchdog/button.
- `tests/`: pure logic tests plus build/resource checks. Hardware timing and wiring need separate recorded bench evidence.

---

### Task 1: Freeze F469 resources and establish a buildable board target

**Files:**
- Create: `boards/stm32f469i_disco/README.md`, `boards/stm32f469i_disco/pin-map.md`, `boards/stm32f469i_disco/CMakeLists.txt`, `boards/stm32f469i_disco/STM32F469NIHX_FLASH.ld`
- Create: `boards/stm32f469i_disco/Core/` startup, system, clock, interrupts, and HAL configuration files from the official STM32CubeF4/Discovery examples for the exact board revision
- Modify: root `CMakeLists.txt` only to add an opt-in F469 target; leave default Wio target behavior intact
- Test: `tests/test_f469_target_manifest.cmake`

**Interfaces:** Consumes official UM1932 and schematic for the actual board revision. Produces CMake target `f469_ui`, a documented connector-to-MCU pin table, and a verified F469 link map for later tasks.

- [ ] **Step 1:** Record the printed Discovery board revision in `pin-map.md`; from the matching ST schematic list UART TX/RX, two timer-capable WS outputs, two lamp GPIOs, user button, LCD/touch resources, expansion connector positions, and every shared board function. Mark a pin usable only after checking alternate functions and solder bridges against the actual board.
- [ ] **Step 2:** Write `test_f469_target_manifest.cmake` to fail if `pin-map.md` lacks all six external signals, the chosen connector pins, or a conflict note, and if F469 build sources contain `stm32h7xx` or the Wio boot stub. Run `cmake -P tests/test_f469_target_manifest.cmake`; expect failure before the board files exist.
- [ ] **Step 3:** Add the opt-in target using STM32CubeF4 CMSIS/HAL and F469 Discovery BSP sources. Keep external SDK paths explicit in CMake configuration, with an error naming the missing package; do not silently pick a different chip or board. Link a minimal main that holds external outputs off.
- [ ] **Step 4:** Configure and build with `cmake -S . -B build-f469 -G Ninja -DBUILD_F469_UI=ON -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake` and `cmake --build build-f469 --target f469_ui`; expect a linked ELF for STM32F469NI and a map showing the intended Flash/RAM/SDRAM regions. Run the manifest check again and expect pass.
- [ ] **Step 5:** Commit only the new board scaffold, the test, and root build changes with `git commit -m "build: add isolated F469 Discovery target"`.

### Task 2: Implement the two-interface navigation state machine

**Files:**
- Create: `App/Inc/ui/two_screen_controller.h`, `App/Src/ui/two_screen_controller.c`, `tests/test_two_screen_controller.c`
- Modify: `tests/CMakeLists.txt`, `tests/test_main.c`

**Interfaces:** Produces `TwoScreenController`, `two_screen_init`, `two_screen_select_main`, `two_screen_swipe`, `two_screen_tick`, and `two_screen_view`. The view contains main interface, information subpage, effective Face mood, and whether selection is manual. Inputs are `VehicleState`, low-battery flag, and millisecond time.

```c
typedef enum { MAIN_FACE, MAIN_INFO } MainScreen;
typedef enum { INFO_OVERVIEW, INFO_DIAGNOSTICS } InfoPage;
typedef struct { MainScreen main; InfoPage info; FaceMood mood; bool manual; } TwoScreenView;
void two_screen_init(TwoScreenController *controller);
void two_screen_select_main(TwoScreenController *controller, MainScreen main);
void two_screen_swipe(TwoScreenController *controller, bool left, uint32_t now_ms);
void two_screen_tick(TwoScreenController *controller, uint32_t now_ms,
                     const VehicleState *state, bool low_battery);
TwoScreenView two_screen_view(const TwoScreenController *controller);
```

- [ ] **Step 1:** Write tests for startup on Face, touch selection of Information, Overview/Diagnostics swipe boundaries, preservation of the current information subpage, three manual Face choices, 10,000 ms expiry, wrap-safe elapsed time, warning override, return from warning before/after expiry, and button fallback only when touch is unavailable. Use explicit times such as 1000, 10999, and 11000 ms.
- [ ] **Step 2:** Add the test source and run `cmake -S tests -B build-host -G Ninja; cmake --build build-host; ctest --test-dir build-host -R unit_tests --output-on-failure`; expect failure because the controller API is absent.
- [ ] **Step 3:** Implement a pure C controller with `typedef enum { MAIN_FACE, MAIN_INFO } MainScreen;`, `typedef enum { INFO_OVERVIEW, INFO_DIAGNOSTICS } InfoPage;` and the existing `FaceMood`. Treat low battery and lost link as display overrides; store the manual choice and timestamp without restarting its timeout during warnings. Do not let a swipe on Information change the Face selection.
- [ ] **Step 4:** Run the same test command and expect pass. Commit the controller and tests with `git commit -m "feat: add two-screen navigation model"`.

### Task 3: Create Overview and Diagnostics view models

**Files:**
- Create: `App/Inc/ui/overview_model.h`, `App/Src/ui/overview_model.c`, `tests/test_overview_model.c`
- Modify: `tests/CMakeLists.txt`, `tests/test_main.c`

**Interfaces:** Produces `overview_model_from_state(const VehicleState *, OverviewModel *)` with fixed-size text fields for battery, throttle, steering, motion, pitch, roll, GPS, armed, and link. Diagnostics consumes the existing `DiagnosticsSnapshot`; no new hardware counters are introduced.

```c
typedef struct {
    char battery[16], throttle[16], steering[16], motion[16];
    char pitch[16], roll[16], gps[16], armed[16], link[16];
} OverviewModel;
void overview_model_from_state(const VehicleState *state, OverviewModel *out);
```

- [ ] **Step 1:** Add tests that supply a valid VehicleState and assert every field, then separately invalidate battery, age attitude/RC/link beyond the existing VehicleState freshness policy, and assert `--` or `UNKNOWN` rather than old values. Assert bounded strings with a small output struct.
- [ ] **Step 2:** Build/run host unit tests and confirm the missing model causes a failure.
- [ ] **Step 3:** Implement the formatting model without LVGL or HAL includes. Use the existing state/link validity rules; keep labels concise enough for the 800×480 layout. Reuse `DiagnosticsSnapshot` directly rather than duplicating counter ownership.
- [ ] **Step 4:** Run host tests to pass and commit with `git commit -m "feat: format overview data with validity"`.

### Task 4: Build native 800×480 Face and Information UI

**Files:**
- Create: `App/Inc/ui/f469_ui.h`, `App/Src/ui/f469_ui.c`, `App/Src/ui/f469_face.c`, `App/Src/ui/f469_info.c`
- Modify: F469 source list in `boards/stm32f469i_disco/CMakeLists.txt`
- Test: `tests/test_f469_ui_manifest.cmake`

**Interfaces:** `f469_ui_init()` creates one root UI with persistent Face/Info tabs; `f469_ui_tick(uint32_t, const VehicleState *, const DiagnosticsSnapshot *, bool low_battery, bool demo)` renders the controller view. Gesture events call the Task 2 controller only; labels consume the Task 3 model.

```c
bool f469_ui_init(void);
void f469_ui_tick(uint32_t now_ms, const VehicleState *state,
                  const DiagnosticsSnapshot *diagnostics,
                  bool low_battery, bool demo);
```

- [ ] **Step 1:** Add a UI manifest test asserting exactly two main interface constructors, Overview and Diagnostics as Information subpages, no `screen_showcase_create` reference, and a visible DEMO label binding. Run it once for expected failure.
- [ ] **Step 2:** Build the 800×480 LVGL hierarchy: 64 px status/tab header, a content region for Face or Information, visible subpage position indicator, and touch targets at least 44 px high. Redraw the five moods at native scale, using the existing `face_model` motion and battery warning policy. Keep animation updates bounded to roughly 30 Hz.
- [ ] **Step 3:** Bind Overview fields and the existing DiagnosticsSnapshot counters. Convert touch tabs to main selection and horizontal gestures to within-main swipes. Bind the on-board user button as a fallback main-interface switch when touch initialization fails. Add a DEMO badge and explicit unknown/stale labels.
- [ ] **Step 4:** Build the F469 target and run `cmake -P tests/test_f469_ui_manifest.cmake`; inspect host-rendered LVGL screenshots or a board display for clipping at 800×480, font coverage, readable warning states, and correct touch target placement. Record images under `boards/stm32f469i_disco/validation/` only if generated from a real render.
- [ ] **Step 5:** Commit UI sources and tests with `git commit -m "feat: redraw two F469 touch interfaces"`.

### Task 5: Bring up display, touch, and MSP on the Discovery board

**Files:**
- Create: `boards/stm32f469i_disco/ports/display_touch.c`, `boards/stm32f469i_disco/ports/msp_uart_f469.c`, matching headers
- Modify: F469 main and source list, plus `boards/stm32f469i_disco/pin-map.md`
- Test: `tests/test_f469_port_manifest.cmake`

**Interfaces:** Display port initializes SDRAM, LTDC/DSI and panel, provides LVGL RGB565 flush and touch input; UART port provides the existing MSP client's byte receive/write contract at 115200 8N1.

```c
bool f469_display_touch_init(void);
bool f469_msp_uart_init(void);
bool f469_msp_uart_read(uint8_t *byte);
bool f469_msp_uart_write(const uint8_t *bytes, size_t count);
```

- [ ] **Step 1:** Write a port manifest that checks framebuffer placement in SDRAM, board BSP touch/display initialization, UART instance/pins against `pin-map.md`, and no H7/Wio include leakage. Run for expected failure.
- [ ] **Step 2:** Initialize panel and touch using the official Discovery BSP matched to the board revision. Allocate the full 800×480 RGB565 scanout (768,000 bytes) in SDRAM, use a bounded LVGL draw buffer, and verify DMA/cache handling for the F4 configuration. Keep display orientation and touch coordinate transform in one port file.
- [ ] **Step 3:** Implement UART RX buffering with overrun accounting and nonblocking TX consistent with the existing MSP client; set 115200 8N1, 3.3 V, TX/RX crossed at the INAV connector, and a separate port from ELRS. Connect real incoming bytes to `vehicle_state_on_msp` and the existing freshness logic.
- [ ] **Step 4:** Build the ELF, run host MSP tests and the manifest, and bench-check actual touch corners and MSP age advancing against a powered INAV UART. If the board is unavailable, record these as pending in the acceptance sheet, not passed.
- [ ] **Step 5:** Commit only port/build/manifest changes with `git commit -m "feat: connect F469 display touch and MSP"`.

### Task 6: Port the two WS2812 chains and two lamp outputs

**Files:**
- Create: `boards/stm32f469i_disco/ports/ws2812_f469.c`, `boards/stm32f469i_disco/ports/lamp_f469.c`, matching headers
- Modify: F469 main/source list and `boards/stm32f469i_disco/pin-map.md`
- Test: `tests/test_f469_lighting_manifest.cmake`, existing `tests/test_ws2812_encoder.c`

**Interfaces:** WS port accepts `Ws2812Frame` and exposes completion/error hooks to the existing `Ws2812Transport`. Lamp port accepts the existing front/roof duty values but applies the existing on/off threshold, with two default-low GPIO outputs.

```c
bool f469_ws2812_init(void);
bool f469_ws2812_submit(uint32_t now_ms, const Ws2812Frame *frame);
void f469_ws2812_complete(void);
void f469_ws2812_error(void);
void f469_lamp_init_off(void);
void f469_lamp_apply(uint16_t front_duty, uint16_t roof_duty);
```

- [ ] **Step 1:** Add transport tests for both chain lengths and ordering (8 and 16), DMA busy rejection, completion, error-stop-to-low, and a later successful submission. Add a manifest that rejects direct GPIO drive of WS2812 or missing default-low lamp initialization; run for expected failure.
- [ ] **Step 2:** Assign the two timer-capable outputs confirmed in Task 1, compute 800 kHz period and 0/1 high times from the actual timer clock, and drive both channels with DMA or another hardware-timed method. Keep a low reset interval at least as long as the existing encoder's 64 slots. On complete/error, force outputs low and update the transport state.
- [ ] **Step 3:** Initialize two lamp GPIOs low before switching to push-pull output. Preserve the existing duty-to-on threshold and AUX6 three-state behavior. Keep AUX8/AUX9 in the portable lighting controller; do not couple UI navigation to them.
- [ ] **Step 4:** Run host lighting/encoder tests and build the F469 target. On hardware, measure both DIN signals after the 74AHCT125 and 330 ohm resistors, then test each chain on a separately limited 5 V rail. Record timing, current, and LED order. Keep 12 V disconnected until lamp MOSFET wiring and gate pulldowns are checked.
- [ ] **Step 5:** Commit with `git commit -m "feat: port two LED chains and lamp switches"`.

### Task 7: Integrate health, startup, and safe fallback

**Files:**
- Modify: F469 main and board ports, `App/Src/app.c` only if required to expose a portable application entry point
- Create: `boards/stm32f469i_disco/ports/watchdog_button.c`, `boards/stm32f469i_disco/validation/acceptance.md`
- Test: `tests/test_f469_integration_manifest.cmake`

**Interfaces:** F469 scheduler feeds the existing MSP client, VehicleState, LightingService, diagnostics and LVGL UI. Watchdog refresh occurs only after MSP, UI and LED progress markers, as in the existing diagnostics gate.

- [ ] **Step 1:** Add an integration manifest for safe startup order: lamp outputs low, WS outputs low, clocks/display initialized, MSP/diagnostics initialized, then watchdog started. Assert no demo vehicle state feeds physical `LightingService` and no old AUX page-selection call remains in the F469 path. Run for expected failure.
- [ ] **Step 2:** Wire the portable application modules to F469 ports. Use the real vehicle state for lighting and the demo-capable presentation state only for UI. Maintain MSP lost/stale timing and low-battery priority. Expose touch failure through DiagnosticsSnapshot and button fallback.
- [ ] **Step 3:** Run all host tests and the F469 build. Populate acceptance.md with explicit PASS/FAIL/PENDING rows for screen, touch, MSP, link loss, both WS chains, two lamp switches, brownout/restart, board power current, and INAV control while UI is unplugged. Mark unperformed physical tests PENDING.
- [ ] **Step 4:** Commit with `git commit -m "feat: integrate F469 UI board runtime"`.

### Task 8: Final verification and handoff

**Files:**
- Modify: `README.md` and `boards/stm32f469i_disco/README.md`, `boards/stm32f469i_disco/validation/acceptance.md`

**Interfaces:** Documents exact build, flash, pin map, protected power, and measured limits for the F469 target; leaves old Wio instructions intact.

- [ ] **Step 1:** Run `cmake -S tests -B build-host -G Ninja`, `cmake --build build-host`, and `ctest --test-dir build-host --output-on-failure`; save the full result summary. Build `f469_ui` from a fresh F469 build directory and inspect the ELF/map for target MCU, memory ranges and symbols.
- [ ] **Step 2:** Inspect `git diff --check`, ensure the new target contains no H7 startup/linker/boot stub, and review the diff against every section of the design spec. Verify unrelated pre-existing uncommitted files remain untouched unless explicitly needed and documented.
- [ ] **Step 3:** On real hardware, follow the acceptance sequence from the spec: board only, touch, INAV MSP, two limited 5 V chains, then MOSFET/12 V lamps. Never mark a physical step PASS without recorded observation. If the board or instruments are unavailable, state that software/build work is complete but vehicle readiness is unverified.
- [ ] **Step 4:** Commit documentation and recorded validation with `git commit -m "docs: document F469 UI bring-up and acceptance"`; provide the user the resulting firmware path, pin map, test evidence and pending physical checks.
