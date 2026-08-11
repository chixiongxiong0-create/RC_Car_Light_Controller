# Task 3 Report: Safe 12 V Lamp GPIO Output Port

## Status

Complete.

## RED Evidence

Added tests/test_lighting_output_port.c first, specifying that duties below
500 are off, 500 and above are on, and an out-of-range UINT16_MAX duty is
treated as clamped high/on. With no implementation linked, the focused host
build failed as expected with:

LNK2019: unresolved external symbol lighting_output_port_duty_is_on

## Implementation

- Added a HAL-free, host-testable duty-to-on helper.
- Added the PF3 (front/D10) and PE10 (roof/D11) active-high GPIO port.
- Initialization enables both clocks, drives both pins low before configuring
  push-pull/no-pull/low-speed outputs.
- Apply writes only with HAL_GPIO_WritePin.
- Added APP_REAR_PIXEL_COUNT and the LED count range guard.
- Wired the source into Make, CMake, and host tests.

## Verification

- Focused GREEN: ctest --test-dir build-host-task3-tdd-red -C Debug -R '^unit_tests$' --output-on-failure — passed (1/1).
- Full host suite: ctest --test-dir build-host-task3-tdd-red -C Debug --output-on-failure — passed (9/9).
- ARM source compile: make bsp_config_seedstudio=1 BUILD_DIR=build_seed_task3_check build_seed_task3_check/lighting_output_port.o — passed with arm-none-eabi-gcc.
- git diff --check — passed.

## Commit

feat: add fail-safe lamp GPIO outputs

## Self-review

Verified PF3/PE10 mapping, clocks and reset-low ordering, GPIO configuration,
host HAL exclusion, duty threshold/clamping behavior, and Make/CMake wiring.
An independent review found no critical, important, or minor issues.

## Concerns

The project CMake ARM toolchain file selected MSVC in this environment, so a
full CMake ARM build could not run. The Make-based ARM GCC compilation of the
new source succeeded.
