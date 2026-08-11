# Task 4 Report: Real MSP Lighting Integration

## Status

Complete.

## RED Evidence

Task 4 introduces no new pure policy behavior: Task 2 already established the
lighting policy through strict RED/GREEN cycles, and Task 4 deliberately keeps
the shared current utilities behaviorally unchanged. The revised LED utility
tests were run before removing the obsolete renderer and passed as
characterization evidence. They now cover exact current estimation, NULL and
zero-count inputs, maximum-count clamping, guard preservation, and post-limit
budget compliance without retaining tests for the removed page-color policy.

## Implementation Summary

- Replaced the legacy page-colored renderer with one static
  `LightingController` and `LightingFrame` in the application.
- Initialized the active-high lamp GPIO port off before initializing any other
  application subsystem, then initialized the pure lighting controller.
- Rendered every physical frame from `real_state`; `presented` remains limited
  to input/UI demo presentation.
- Applied front and roof duties on every tick, so startup, malformed, stale,
  and lost-link frames actively write both outputs off.
- Submitted the controller's combined, current-limited pixel frame through the
  existing non-blocking SPI3 WS2812 port and reported its post-limit
  `estimated_ma` to diagnostics.
- Removed obsolete legacy LED policy/types while retaining `LedRgb`,
  `LED_MAX_PIXELS`, `LED_CURRENT_BUDGET_MA`, `led_estimated_ma`, and
  `led_limit_current`.
- Added `lighting_controller.c` to both firmware build systems exactly once and
  completed the deferred public-header cleanup in the output-port test.

## Exact Verification

- Focused host: `cmake --build build-host-task4-baseline --target unit_tests`
  followed by `ctest --test-dir build-host-task4-baseline -C Debug -R
  '^unit_tests$' --output-on-failure` - passed 1/1.
- Fresh full host: `cmake -S tests -B build-host-task4-final -G Ninja`,
  `cmake --build build-host-task4-final`, and `ctest --test-dir
  build-host-task4-final --output-on-failure` - passed 9/9.
- Fresh short-path ARM Make build from junction `E:/workspace/fpv/r4`:
  `make bsp_config_seedstudio=1 BUILD_DIR=b5 -j4` - exit 0; generated
  `wio_ai.elf` (6,279,776 bytes), `wio_ai.hex` (959,747 bytes), and
  `wio_ai.bin` (341,188 bytes).
- `arm-none-eabi-objdump -h b5/wio_ai.elf` reports `.isr_vector` VMA
  `08020000`; `arm-none-eabi-nm` confirms the lighting controller, GPIO apply,
  and WS2812 submit symbols are linked.
- `git diff --check` - passed.

## Commit

`feat: integrate MSP lighting outputs`

## Self-review

Reviewed the Task 4 brief line by line and inspected the complete scoped diff.
Confirmed real/demo state separation, raw low-battery and link inputs, board
fault preservation, active-off application on every tick, combined pixel
ordering/count, post-limit diagnostics, non-blocking submission, unchanged UI,
display, backlight, USART3, callback, and boot-vector paths, and exact-once
source entries in Make, firmware CMake, and host CMake. No unrelated tracked
changes are included.

## Concerns

The ARM build retains pre-existing unused BSP declaration/variable warnings and
the linker warning that the ELF has an RWX LOAD segment. No Task 4 warnings or
errors were introduced. Physical wiring and polarity still require the planned
bench acceptance before lamps are permanently connected.
