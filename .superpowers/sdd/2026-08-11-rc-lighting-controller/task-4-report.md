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

## Review Fix Round 1: Host-Testable Application Wiring

### RED Evidence

Added `tests/test_lighting_service.c` and the public service interface before
adding any implementation. The focused command
`cmake --build build-host-task4-final --target unit_tests` failed at link time
with undefined references to `lighting_service_init` and
`lighting_service_tick`. This proved the tests required the missing integration
seam rather than passing against the pre-existing direct application wiring.

### Implementation

- Added `LightingService`, which owns one `LightingController` and its current
  `LightingFrame` and accepts exactly one `VehicleState` argument per tick.
- The service renders with the real policy, invokes the apply callback once on
  every tick (including zero/zero for starting, stale, and lost links), submits
  the exact same combined frame and count once, and returns the frame's
  post-limit `estimated_ma`.
- Replaced the direct application wiring with thin non-blocking callbacks around
  `lighting_output_port_apply` and `ws2812_port_submit`; `App_Tick` passes only
  `real_state` to the physical service.
- Registered `lighting_service.c` exactly once in host CMake, firmware CMake,
  and Make builds.

### Verification

- Focused GREEN: `cmake --build build-host-task4-final --target unit_tests`
  followed by `ctest --test-dir build-host-task4-final -R '^unit_tests$'
  --output-on-failure` - passed 1/1.
- Fresh full host: `cmake -S tests -B build-host-task4-fix1 -G Ninja`,
  `cmake --build build-host-task4-fix1`, and `ctest --test-dir
  build-host-task4-fix1 --output-on-failure` - passed 9/9.
- Fresh short-path ARM Make build from `E:/workspace/fpv/r4`:
  `make bsp_config_seedstudio=1 BUILD_DIR=b6 -j4` - exit 0; generated
  ELF (6,284,436 bytes), HEX (960,046 bytes), and BIN (341,292 bytes).
- `.isr_vector` remains at `0x08020000`; the ELF contains
  `lighting_service_init`, `lighting_service_tick`,
  `lighting_output_port_apply`, and `ws2812_port_submit`.

### Self-review and Concerns

The tests use the real lighting controller and fake only the two hardware
boundaries. Literal assertions cover valid AUX duty flow, rear pixels 0..3,
roof pixels 4..N-1, exact submission count/call count, frame identity, returned
diagnostic current, and active-off writes on each starting/stale/lost tick.
There is no demo/presented parameter in the service API. Existing BSP and RWX
linker warnings remain; no new warning or physical-hardware concern was added.
