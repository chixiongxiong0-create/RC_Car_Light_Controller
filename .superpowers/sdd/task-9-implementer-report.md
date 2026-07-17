# Task 9 Implementer Report

## Outcome

Implemented Showcase C and the unified A/B/C page transition controller.

- Showcase is built entirely from LVGL text and vector/object primitives.
- All three screens are allocated once during `ui_app_init()`.
- Only the actually visible page receives data updates.
- Page requests come from `input_manager_page()`; AUX priority remains owned by
  the already-tested input manager.
- Page changes use a 100 ms yellow/black cover, swap while fully covered, and a
  100 ms retract. New requests use latest-request-wins semantics without ever
  unloading the currently visible screen.

## TDD Evidence

RED was observed with the new showcase test registered before production code:
CMake failed because `App/Src/ui/showcase_model.c` did not exist.

GREEN covers:

- exact 4000 ms idle rotation boundaries;
- exact 1500 ms arming-edge hold and repeated armed samples not restarting it;
- strict `abs(throttle) > 0.2` motion threshold;
- steering-sign stripe direction;
- 32-bit tick wrap-around;
- 100 ms cover/retract boundaries;
- latest request during a transition, no-op requests, and transition tick wrap.

## Verification

Host tests:

```text
ctest --test-dir build-host-task9 -C Debug --output-on-failure
100% tests passed, 0 tests failed out of 3
```

CMake Debug gate:

```text
cmake --preset debug
cmake --build --preset debug
text=369408 data=1880 bss=581568
Flash (text + data)=371288 bytes
384 KiB gate headroom=21928 bytes (21.4 KiB)
```

The Debug gate therefore retains 1448 bytes more than the required 20 KiB
headroom. No UI or safety-logic trimming was required.

Filtered Make build:

```text
make bsp_config_seedstudio=1 BUILD_DIR=build_seed_task9 OPT=-Os -j8
text=325644 data=564 bss=581120
Flash (text + data)=326208 bytes
384 KiB gate headroom=67008 bytes (65.4 KiB)
```

`git diff --check` reports no whitespace errors.

## Concerns / Deferred Hardware Evidence

- Physical no-blank-frame behavior and FPS need target-board observation.
- A separate Release-only CMake experiment exposed two pre-existing build-chain
  differences: non-Debug BSP include directories are incomplete, and after a
  temporary local include-path experiment its all-source LVGL glob linked far
  more code than the filtered Make manifest and overflowed FLASH/RAM. The
  temporary include-path experiment was removed; Task 9 does not alter that
  unrelated Release configuration. The required CMake Debug and Make builds
  both pass.
