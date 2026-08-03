# Automatic Demo Vehicle State Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Animate all UI pages before the first valid MSP frame, then permanently and safely hand control to real vehicle data.

**Architecture:** A deterministic `demo_vehicle_state` generator remains independent of MSP. A `vehicle_state_source` selector owns the one-way real-data latch and 300 ms numeric blend; `app.c` passes the selected state to every presentation consumer and passes a separate demo flag to the UI.

**Tech Stack:** C11, STM32H725 HAL, LVGL 9, CMake/CTest host tests, GNU Arm Embedded Make build.

## Global Constraints

- Do not inject synthetic MSP frames or modify MSP parser/link timeout semantics.
- Demo is active only until the first accepted MSP frame of the current boot.
- A later stale/lost link must never reactivate demo.
- Demo `aux_page` is exactly `0.0f`; `USER1` remains the page selector.
- Demo is always unarmed and visibly labeled `DEMO`.
- Blend numeric fields for 300 ms; switch discrete safety/link fields immediately.
- Preserve existing LTDC, OSPI/PSRAM, UART3, SPI3, PLL, and watchdog configuration.
- The PSRAM linker script places the application at `0x08020000`; HEX is the preferred programming artifact.

---

## File Structure

- `App/Inc/demo_vehicle_state.h`, `App/Src/demo_vehicle_state.c`: deterministic bounded synthetic data only.
- `App/Inc/vehicle_state_source.h`, `App/Src/vehicle_state_source.c`: demo/real selection, one-way latch, and blend.
- `App/Src/app.c`: notify selector after an accepted MSP frame and use one selected state for UI/input/LED policy.
- `App/Inc/ui/ui_app.h`, `App/Src/ui/ui_app.c`: receive demo metadata and render a persistent badge.
- `tests/test_demo_vehicle_state.c`, `tests/test_vehicle_state_source.c`: host behavior tests.
- `tests/CMakeLists.txt`, `Makefile`, `CMakeLists.txt`: compile new modules in host and firmware targets.
- `README.md`: correct programming artifact and address guidance.

### Task 1: Deterministic demo generator and real-data selector

**Files:**
- Create: `App/Inc/demo_vehicle_state.h`
- Create: `App/Src/demo_vehicle_state.c`
- Create: `App/Inc/vehicle_state_source.h`
- Create: `App/Src/vehicle_state_source.c`
- Create: `tests/test_demo_vehicle_state.c`
- Create: `tests/test_vehicle_state_source.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `void demo_vehicle_state_sample(uint32_t now_ms, uint8_t battery_cells, VehicleState *out)`.
- Produces: `VehicleStateSource`, `vehicle_state_source_init`, `vehicle_state_source_note_real`, `vehicle_state_source_tick`, `vehicle_state_source_get`, and `vehicle_state_source_is_demo`.
- Consumes: read-only real `VehicleState` samples from `vehicle_state_get()`.

- [ ] **Step 1: Write failing demo-generator tests**

Add tests that sample `0`, `1000`, `5000`, and `12000` ms and assert: controls stay in `[-1,1]`; heading stays in `[0,360)`; roll/pitch remain within `±20` degrees; `aux_page == 0`; `armed == false`; battery is above `3.7 V * cells` when cells are known; at least throttle, steering, roll, pitch, heading, RSSI, and satellites change across the samples; repeated timestamps produce identical structs.

- [ ] **Step 2: Run the host test and verify RED**

Run: `cmake -S tests -B build-host-demo && cmake --build build-host-demo && ctest --test-dir build-host-demo --output-on-failure`

Expected: compilation fails because `demo_vehicle_state.h` and its function do not exist.

- [ ] **Step 3: Implement the bounded deterministic generator**

Use integer phase calculations plus `sinf` only where smooth motion is required. Initialize every field, set `link = LINK_STARTING`, keep `aux_page = 0.0f` and `armed = false`, and choose nominal battery voltage from `battery_cells` without triggering the low-battery policy.

- [ ] **Step 4: Write failing selector tests**

Cover boot demo selection, invalid/no notification leaving demo enabled, `vehicle_state_source_note_real(source, now_ms)` permanently latching real ownership, immediate real values for `link/armed/mode_flags/gps_sats`, numeric interpolation at 0/150/300 ms, and later `LINK_LOST` remaining real rather than returning to demo.

- [ ] **Step 5: Run the selector tests and verify RED**

Run the same CMake/CTest command. Expected: compilation fails because `vehicle_state_source` interfaces do not exist.

- [ ] **Step 6: Implement the selector minimally**

Define:

```c
typedef struct {
    bool real_seen;
    bool blending;
    uint32_t blend_started_ms;
    VehicleState demo;
    VehicleState blend_from;
    VehicleState selected;
} VehicleStateSource;

void vehicle_state_source_init(VehicleStateSource *source);
void vehicle_state_source_note_real(VehicleStateSource *source, uint32_t now_ms);
void vehicle_state_source_tick(VehicleStateSource *source, uint32_t now_ms,
                               uint8_t battery_cells,
                               const VehicleState *real_state);
const VehicleState *vehicle_state_source_get(const VehicleStateSource *source);
bool vehicle_state_source_is_demo(const VehicleStateSource *source);
```

Snapshot the current demo sample at takeover. Interpolate only throttle, steering, roll, pitch, heading, and battery; blend heading along the shortest wrapped arc. Copy `aux_page`, RSSI, mode flags, satellites, armed, link, and timestamps from real state immediately. Finish blending when unsigned elapsed time reaches 300 ms.

- [ ] **Step 7: Run tests and commit**

Run the host test command and require all CTest entries to pass.

Commit:

```bash
git add App/Inc/demo_vehicle_state.h App/Src/demo_vehicle_state.c App/Inc/vehicle_state_source.h App/Src/vehicle_state_source.c tests/test_demo_vehicle_state.c tests/test_vehicle_state_source.c tests/CMakeLists.txt
git commit -m "feat: add safe automatic demo vehicle state"
```

### Task 2: Integrate selector and persistent DEMO badge

**Files:**
- Modify: `App/Src/app.c`
- Modify: `App/Inc/ui/ui_app.h`
- Modify: `App/Src/ui/ui_app.c`
- Modify: `Makefile`
- Modify: `CMakeLists.txt`
- Test: `tests/test_button_polling_manifest.ps1`

**Interfaces:**
- Consumes: all Task 1 selector interfaces.
- Changes: `void ui_app_tick(uint32_t now_ms, const VehicleState *state, bool low_battery, bool demo_active)`.
- Produces: one selected state shared by input manager, low-battery policy, UI, diagnostics-facing age logic where applicable, and LED rendering.

- [ ] **Step 1: Add a failing build/manifest assertion**

Extend the existing manifest test or add a small PowerShell manifest test asserting both new source files are present in the firmware source lists and that `app.c` calls `vehicle_state_source_note_real` only after `vehicle_state_on_msp` returns true.

- [ ] **Step 2: Run CTest and verify RED**

Run the host CMake/CTest command. Expected: manifest assertion fails before integration.

- [ ] **Step 3: Integrate the selector in `app.c`**

Initialize a static `VehicleStateSource`. In `on_msp_frame`, call `vehicle_state_on_msp`; only on `true` call `vehicle_state_source_note_real`. Each `App_Tick`, update the real state first, tick the selector, store one `const VehicleState *presented`, and use that pointer consistently for input, battery, UI, and LED rendering. Preserve diagnostics MSP age/timeouts from the real client/state rather than demo values.

- [ ] **Step 4: Add the persistent badge**

Create a small top-layer LVGL label in `ui_app_init`, style it with a distinct non-alarm color, and toggle `LV_OBJ_FLAG_HIDDEN` from the new `demo_active` argument. Keep it visible across all three screens and hide it immediately on real takeover.

- [ ] **Step 5: Run host tests and firmware builds**

Run:

```powershell
cmake -S tests -B build-host-demo
cmake --build build-host-demo
ctest --test-dir build-host-demo --output-on-failure
make BUILD_DIR=build_seed_demo bsp_config_seedstudio=1 -j4
```

Expected: all CTest entries pass; `build_seed_demo/wio_ai.elf`, `.hex`, and `.bin` are generated.

- [ ] **Step 6: Commit integration**

```bash
git add App/Src/app.c App/Inc/ui/ui_app.h App/Src/ui/ui_app.c Makefile CMakeLists.txt tests
git commit -m "feat: animate UI before first MSP data"
```

### Task 3: Correct programming documentation and final verification

**Files:**
- Modify: `README.md`
- Modify: `docs/hardware/acceptance-results.md`

**Interfaces:**
- Consumes: Task 2 firmware artifacts.
- Produces: unambiguous programming instructions that cannot place a raw BIN at the wrong address.

- [ ] **Step 1: Correct programming guidance**

State that `wio_ai.hex` is preferred because it carries addresses. State that this build's `STM32H725AEIX_PSRAM.ld` places `.isr_vector` at `0x08020000`, so a raw BIN must be written at exactly `0x08020000`. Remove the incorrect `0x08000000` instruction. Warn against full-chip erase until the intended boot layout is deliberately finalized.

- [ ] **Step 2: Record the hardware discovery**

Add an acceptance note with the J-Link evidence: wrong BIN placement prevented application entry; correct addressed HEX programming produced normal execution and PF5 high. Keep visual demo animation acceptance pending until observed by the user.

- [ ] **Step 3: Perform fresh verification**

Run a fresh host CMake build/CTest and fresh firmware Make build. Inspect the ELF with `arm-none-eabi-objdump -h` and require `.isr_vector` VMA `08020000`. Inspect the HEX first extended address record or use `arm-none-eabi-objdump` to confirm addressed output. Run `git diff --check`.

- [ ] **Step 4: Commit documentation**

```bash
git add README.md docs/hardware/acceptance-results.md
git commit -m "docs: correct STM32 application programming address"
```

- [ ] **Step 5: Hardware handoff**

Program the fresh HEX with J-Link, run without halting, and confirm via registers that the CPU is in `0x0802xxxx..0x0807xxxx` and PF5 output is high. Leave these visual checks explicitly pending for the user: animated values on all three pages, `USER1` page cycling, visible `DEMO` badge, and live MSP takeover.
