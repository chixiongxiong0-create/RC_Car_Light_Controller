# Final review fix report

Date: 2026-08-04

Base commit: `09f5be2`

Scope: all binding fixes in `final-fix-brief.md`

Result: software implementation and artifact verification PASS. A later J-Link follow-up verified stable-stub programming, reset handoff, short-run execution, and PF5 high; visual, live-MSP, 60-second, and soak acceptance remain PENDING.

## Implemented fixes

### Raw low-battery safety

- Added `VehicleState.battery_valid`; only an accepted `MSP_ANALOG` frame sets it.
- `App_Tick()` now evaluates low battery through `low_battery_policy_update_from_vehicle()` using `vehicle_state_get()->battery_v` and `battery_valid`, never the blended presentation voltage.
- Demo ownership explicitly clears/disables the low-battery result.
- Tests cover Demo with a previously latched low input, RC-only takeover, missing ANALOG, delayed low ANALOG, first-tick queued low ANALOG, and first-tick queued healthy ANALOG.

### Continuous heading takeover

- `VehicleStateSource` keeps the first Demo origin fixed while accumulating a continuously unwrapped real heading target relative to the previous real target.
- The presented heading is interpolated to that continuous target and normalized only at output.
- Tests cover `179 -> 181`, `181 -> 179`, `350 -> 10`, `10 -> 350`, exact 180-degree cases from both sides and from the origin, the 299/300 ms boundary, and unsigned 32-bit time wrap.

### Demo timestamps

- Demo samples no longer populate `last_msp_ms`, `last_rc_ms`, or `last_attitude_ms`; all remain zero because Demo is not MSP evidence.

### ARM build layout and math linkage

- Root ARM CMake now selects the non-overridable `STM32H725AEIX_PSRAM.ld` for every configuration and links `libm`.
- Root CMake includes the SeedStudio BSP include paths needed by a fresh Release build.
- The existing SeedStudio Make path remains on the PSRAM linker script and continues to link `-lm`.

### Stable sector-0 boot stub

- Added a standalone, tracked assembly stub with a 16-word Cortex-M core vector table at `0x08000000` and fixed Thumb reset entry at `0x08000040`.
- The reset entry dynamically reads application MSP and `Reset_Handler` from `0x08020000`, writes VTOR, executes `dsb`/`isb`, installs MSP, and branches through the loaded register. It does not embed an application reset address and does not mask interrupts.
- Added independent Make and CMake builds, linker assertions, a tracked addressed HEX, a host manifest, and an artifact inspector that compares fresh output with the tracked HEX and rejects the current application reset address in the stub.

### Documentation and acceptance claims

- README now requires both the stable stub HEX and addressed application HEX for blank, erased, or legacy-vector devices; it states that application HEX alone cannot boot such a device.
- Raw BIN addresses are documented as `0x08000000` for the stub and `0x08020000` for the application.
- The prior 3-second snapshot plus final `go` is recorded only as a legacy static-vector short-run PASS. A later stable-stub follow-up is recorded separately as hardware boot and short-run PASS; 60-second display observation and the four-hour soak remain PENDING.

## TDD evidence

Tests were introduced or expanded before the related production changes.

- Low-battery RED: the focused build initially failed because raw validity and `low_battery_policy_update_from_vehicle()` did not exist; the manifest also found that `App_Tick()` still consumed `presented->battery_v`.
- Demo timestamp RED: the new assertions observed nonzero synthetic MSP timestamps before the generator assignments were removed.
- Heading RED: the `179 -> 181` evolving-target case produced `270.5` degrees instead of `90.5`, demonstrating the antipode branch flip.
- Boot/layout RED: the manifest initially failed because no stable stub existed; the first CMake stub link also exposed the ASM-only linker-driver issue, fixed by selecting the C linker driver.
- Release build RED: a fresh root Release build exposed missing SeedStudio BSP include paths; explicit target includes fixed it.
- Final-review RED: the boot manifest rejected the overridable CMake linker-script cache entry; it also required the inspector to verify the application section layout before reporting PASS.

All focused tests passed after the corresponding changes, followed by the full clean suite below.

## Final verification

### Host tests

Fresh directory `build-host-final-fix`:

- configure: PASS
- build: PASS
- CTest: PASS, 9/9, 100%
- includes unit, Demo-state, selector, low-battery integration, Demo safety manifest, boot-stub manifest, and the existing LVGL/button/watchdog manifests

### Fresh SeedStudio Make build

Command used a fresh `build_seed_final_fix` directory and the available GNU Arm toolchain:

- result: PASS, exit 0
- size: text `339272`, data `564`, bss `584576`
- `.isr_vector`: `0x08020000`
- application HEX begins with extended address record `:020000040802F0`
- `Reset_Handler`: `0x0805CD84`
- `sinf`: defined at `0x0805F950`; no unresolved `sinf`

### Root ARM CMake

Fresh Ninja builds with the GNU Arm toolchain:

- Debug: PASS; text `353904`, data `1864`, bss `585056`; `.isr_vector=0x08020000`; `Reset_Handler=0x08024CA4`; `sinf` defined at `0x0805D420`
- Release: PASS; text `353104`, data `1864`, bss `585056`; `.isr_vector=0x08020000`; `Reset_Handler=0x08024C98`; `sinf` defined at `0x0805D100`
- no unresolved `sinf` in either linked application

### Boot-stub artifact inspection

- fresh Make stub: PASS; text `100` bytes
- vector table VMA/LMA: `0x08000000`
- `BootStub_Reset_Handler`: fixed at `0x08000040`, vector word is `0x08000041`
- disassembly confirms loads from `[0x08020000]` and `[0x08020004]`, VTOR write, barriers, `msr MSP`, and register-indirect branch
- fresh HEX records are identical to `boot_stub/wio_ai_boot_stub.hex` after newline normalization (the Windows build emits CRLF while the tracked source artifact uses repository line endings)
- inspector passed against Make, CMake Debug, and CMake Release applications; current reset addresses `0x0805CD84`, `0x08024CA4`, and `0x08024C98` are not embedded
- inspector explicitly rejects any application whose `.isr_vector` VMA/LMA is not `0x08020000`

### Repository checks

- `git diff --check`: PASS, exit 0 (line-ending conversion warnings only)
- unrelated modified/untracked files were preserved and excluded from the commit scope
- no hardware was flashed, erased, reset, or otherwise programmed in this fix wave

## Hardware follow-up evidence (2026-08-04)

After the software-only fix wave, a controller used J-Link V8.18 to program the tracked stable stub and fresh addressed application. Both writes reported `Verify O.K.`.

- Sector-0 readback: initial SP `0x24050000`, reset vector word `0x08000041`; the fixed code at `0x08000040` dynamically reads the application vector.
- Application-vector readback at `0x08020000`: `0x24050000 / 0x0805CD85` (Thumb bit included in the reset vector word).
- Three seconds after reset: `PC=0x08023E88`, `IPSR=0`, `VTOR=0x08020000`.
- Later runtime probe: `PC=0x0805EE78`, `IPSR=0`, GPIOF `ODR=0x20` (PF5 high), `CFSR=0`, and `HFSR=0`.
- Both scripts ended with `go`.

This evidence marks stable-stub programming/readback, reset entry into the fresh application, short-run CPU execution, and PF5 high as PASS. It does not establish continuous or long-duration operation.

## Remaining concerns and required hardware follow-up

1. Stable-stub hardware boot evidence is limited to successful programming/verification, vector readback, reset handoff, and two short live snapshots. It does not establish continuous or long-duration operation.
2. Programming sector 0 erases a 128 KiB sector. Before doing so, confirm it contains no other bootloader, calibration, or persistent data and confirm option bytes boot from `0x08000000`. Do not mass erase.
3. The existing ATTITUDE filter in `vehicle_state.c` performs ordinary scalar heading low-pass filtering. The required selector takeover blend is now wrap-continuous, but a separate end-to-end requirement for wrap-aware MSP heading filtering would need its own change and tests.
4. The existing application link still reports an RWX LOAD-segment warning, and the fresh Make build reports existing unused/static-declaration warnings. Neither caused a build or test failure in this scope.
5. Visual UI/Demo behavior, `USER1`, the visible `DEMO` badge, live MSP takeover, 60-second dynamic display observation, and four-hour soak remain PENDING.
