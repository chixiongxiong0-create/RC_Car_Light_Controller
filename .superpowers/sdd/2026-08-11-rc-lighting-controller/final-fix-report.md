# RC Lighting Controller Final Fix Report

## Scope

This final review wave addressed three safety and configuration findings without flashing hardware:

1. Lighting control previously trusted the aggregate MSP link state, so unrelated telemetry could keep stale AUX6-AUX9 values actionable after complete `MSP_RC` frames stopped.
2. The controller returned an all-black frame for every invalid link and could not distinguish startup from loss after a previously valid complete lighting RC frame.
3. `APP_REAR_PIXEL_COUNT` appeared configurable even though the renderer and physical rear layout are fixed at four pixels.

The pre-existing line-ending-only modification to `docs/superpowers/specs/2026-07-17-rc-crawler-decoration-ui-design.md`, J-Link files, and earlier build directories were preserved and excluded from this change.

## Root cause and implementation

### Independent complete-lighting-RC validity

`VehicleState` now carries `lighting_rc_valid` and `last_lighting_rc_ms`. Only a complete, successfully decoded `MSP_RC` payload of at least 26 bytes commits AUX6-AUX9, sets the validity bit, and records the independent timestamp. A shorter otherwise-valid RC frame still updates the backward-compatible steering/throttle/page state and generic `last_rc_ms`, but immediately clears lighting validity and resets AUX6-AUX9 to `-1.0f`.

`vehicle_state_tick()` uses unsigned subtraction to invalidate lighting RC at 500 ms independently of `last_msp_ms`. The regression test starts the full RC timestamp near `UINT32_MAX`, feeds `MSP_ANALOG` after wrap so the aggregate link remains `LINK_OK`, and proves lighting validity and AUX values are cleared after 501 ms.

### Startup and post-valid loss behavior

`LightingController` now latches `has_seen_valid_lighting_rc` only after rendering a state whose aggregate link is `LINK_OK` and whose complete lighting RC validity is true. Normal running, reverse, turn, brake, warning, roof modes, and both high-power duties are behind this combined gate.

Before the first complete valid lighting RC frame, invalid, stale, or lost state renders all black. After at least one valid frame, any invalid lighting RC or aggregate link state forces both duties to zero and the roof region black, then renders only the four rear pixels as low-current amber `{32, 8, 0}` during `[0,100)` and `[200,300)` of each 2000 ms period. Board-fault, battery, brake, reverse, turn, and decorative state cannot override this loss branch.

### Fixed rear layout contract

`app_config.h` now rejects every `APP_REAR_PIXEL_COUNT` value other than 4 and validates total pixels directly as 4 through `LED_MAX_PIXELS` (30). A portable nested CMake compile test checks valid totals 4 and 30 and rejects rear count 5, total 3, and total 31. The README, design, and hardware checklist state that the first four pixels are fixed rear pixels.

## TDD evidence

All production changes followed observed RED failures before GREEN implementation:

| Contract | RED evidence |
|---|---|
| State exposes independent validity | Host compile failed because `VehicleState` had no `lighting_rc_valid` or `last_lighting_rc_ms` members. |
| Complete RC establishes validity | `unit_tests` failed at `assert(state->lighting_rc_valid)` in `test_decodes_lighting_aux_channels`. |
| Short RC invalidates immediately | `unit_tests` failed at `assert(!state->lighting_rc_valid)` in `test_short_rc_frame_resets_lighting_aux_channels_to_safe_value`. |
| Complete RC expires while other MSP continues, including clock wrap | `unit_tests` failed at `assert(!state->lighting_rc_valid)` after the wrapped 501 ms interval while `link == LINK_OK`. |
| Startup requires complete lighting RC and post-valid loss has its own pattern | `unit_tests` failed in `assert_frame_off()` because the pre-fix renderer energized the front duty when `lighting_rc_valid` was false. |
| Rear count is exactly four | `app_config_compile_contract` failed with `invalid_rear_override should be rejected but compiled`. |

Focused GREEN checks passed after each minimal implementation step. The final focused `unit_tests` target and `app_config_compile_contract` test both passed.

## Final verification

### Fresh host build

Commands:

```text
cmake -S tests -B build-host-final-safety -G Ninja
cmake --build build-host-final-safety
ctest --test-dir build-host-final-safety --output-on-failure
```

Result: 11/11 tests passed, 0 failed. This includes unit, source-selection, manifest, lighting wiring, boot-stub, and compile-contract coverage.

### Fresh short-path ARM build

The worktree was exposed through the short junction `E:/workspace/fpv/r9`, then built with:

```text
make -C E:/workspace/fpv/r9 bsp_config_seedstudio=1 BUILD_DIR=b9 -j4
```

Result: exit 0. Generated artifacts:

- `b9/wio_ai.elf`: 6,285,908 bytes
- `b9/wio_ai.hex`: 960,197 bytes
- `b9/wio_ai.bin`: 341,348 bytes

`arm-none-eabi-objdump -h b9/wio_ai.elf` reports `.isr_vector` VMA and LMA at `0x08020000`. `arm-none-eabi-nm --undefined-only b9/wio_ai.elf` reports zero unresolved symbols.

The ARM build retains the known BSP warnings for unused camera declarations, an unused OSPI calibration variable, and the linker RWX LOAD segment warning. No new warning was associated with this fix.

### Repository checks

`git diff --check` passed. The scoped source and test diff was manually reviewed for validity gating, atomic AUX state updates, unsigned timeout arithmetic, loss priority, bounded pixel writes, current accounting, and compile-test portability.

## Remaining concern

Physical acceptance remains pending exactly as documented in `docs/hardware/lighting-checklist.md`. PF3/PE10 polarity and header identity, AUX channel order, steering/throttle polarity, rear pixel order, strip length, current behavior, and visible loss timing still require safe bench verification. No hardware was flashed during this wave.
