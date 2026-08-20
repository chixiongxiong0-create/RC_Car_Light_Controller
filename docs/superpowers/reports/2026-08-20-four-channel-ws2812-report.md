# Four-channel WS2812 verification report — 2026-08-20

## Status and hardware boundary

The final-fix wave for the four independent WS2812 outputs (4/4/8/8 pixels)
is software-verified at commit `605949a`. The specified interface remains
`100 Ω -> 74AHCT125 -> 330 Ω -> DIN` on each output.

**No flash was performed. No hardware was connected or operated.** Every
physical check remains `PENDING`: T0H/T1H/period/reset-low measurements at all
four 74AHCT125 outputs, current and 5 V droop, thermal behavior, WS1/WS2
left/right identity, WS3/WS4 placement, startup/reset behavior, and MSP
stale/link-loss behavior. Software results are not physical acceptance.

TIMPRE analysis remains explicitly deferred; this wave did not change it.

## Final-fix scope

- PWM compares are now derived from the actual timer clock using rounded
  350 ns and 700 ns targets. Host coverage proves a 275 MHz clock and 344-tick
  period produce compare values 96 and 193, with zero/out-of-period validation.
  Production initialization obtains the clock through
  `ws2812_timer_clock_hz()` and uses the same tested calculation.
- Obsolete SPI3/PB5 WS2812 support is retired. `Core/Src/spi.c` and
  `Core/Inc/spi.h` are removed; Make/CMake no longer compile the source; SPI3
  handle, PB5 AF6, DMA1 Stream1, NVIC/IRQ, and Cube SPI3/SPI3_TX metadata are
  gone. PB5 remains `TIM3_CH2`. HAL SPI and real SPI1 display support remain.
- `Ws2812Transport.busy_drop_count` records only valid candidate frames
  rejected because the transport state is non-IDLE, including the protected
  race-window check. Invalid encoding/capacity, the normal 34 ms rate limit,
  and start/error paths do not increment it. It is exposed separately through
  `ws2812_transport_busy_drops()`, `ws2812_port_busy_drops()`, and
  `DiagnosticsSnapshot.ws2812_busy_drops`; it is not the transport error count.
- The timer manifest now checks DMA1 Streams 2/3, TIM2_UP/TIM3_UP requests,
  priority 5/0, completion/error callbacks, PA2 exclusion, PB5 TIM3_CH2
  ownership, exact source wiring, and absence of stale SPI3 ownership.

## TDD RED/GREEN evidence

Each Important finding received an explicit failing check before its
production change:

| Finding | RED evidence | GREEN evidence |
|---|---|---|
| 350/700 ns timing | Host link failed with unresolved `ws2812_compare_ticks`; timer manifest failed because `ws2812_timer_clock_hz()` was not bound | Unit test proves 275 MHz -> 96/193 and invalid ranges; target manifest binds production clock use and rejects the old 1/3, 2/3 calculations |
| SPI3 retirement | `ws2812_timer_manifest` failed with `Obsolete Core/Src/spi.c still exists` | Manifest passes after complete source/MSP/DMA/IRQ/Cube retirement while preserving PB5 TIM3_CH2 and SPI1 |
| busy-drop diagnostic | Host compile failed because `DiagnosticsSnapshot.ws2812_busy_drops` did not exist; application manifest failed because no observable accessor was wired | Unit tests distinguish busy, invalid, rate-limit, race, start-failure, and error cases; diagnostics and application manifests pass |

Focused final command:

```powershell
ctest --test-dir build-host-ws4-final -C Debug `
  -R "unit_tests|lighting_app_wiring|ws2812_timer_manifest" `
  --output-on-failure
```

Result: exit 0; **3/3 focused tests passed**.

## Authoritative fresh host suite

A new directory, `build-host-ws4-final-fix/`, was absent before configuration.
The authoritative CTest command for this Visual Studio multi-configuration
generator is the command containing `-C Debug`:

```powershell
cmake -S tests -B build-host-ws4-final-fix
cmake --build build-host-ws4-final-fix --config Debug
ctest --test-dir build-host-ws4-final-fix -C Debug --output-on-failure
```

Configure, build, and CTest each exited 0. **12/12 registered tests passed
(100%)**. The non-`-C` CTest spelling is not authoritative in this workspace.

## Genuinely fresh ARM build and artifacts

`b4f1/` did not exist before the command below. Its first full invocation
compiled all target and LVGL objects, linked, and generated ELF/HEX/BIN:

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=b4f1 -j4
```

Result: exit 0. The build emitted only the pre-existing BSP unused-symbol
warnings and the pre-existing linker RWX LOAD-segment warning.

| Artifact | Size (bytes) | SHA-256 |
|---|---:|---|
| `b4f1/wio_ai.elf` | 6,941,640 | `A15FC711E336F547F3537C57675CAC4EF187D1EC451905C5440D55E131468D51` |
| `b4f1/wio_ai.hex` | 971,087 | `3B6FAFC9F84164873D05897BADD56E9E4A5D87C8CE1D2E01A0FAA8C05B43266A` |
| `b4f1/wio_ai.bin` | 345,220 | `005653E379B230CEE75279D84F4033B90AF2E15B6F4E6C8E9C3B6674030F6C8C` |

Inspection commands and results:

```powershell
arm-none-eabi-objdump -h b4f1/wio_ai.elf
arm-none-eabi-nm -u b4f1/wio_ai.elf
Get-FileHash b4f1/wio_ai.elf,b4f1/wio_ai.hex,b4f1/wio_ai.bin -Algorithm SHA256
```

- `.isr_vector` VMA/LMA: `0x08020000`.
- `arm-none-eabi-nm -u`: empty output, unresolved count 0.
- All three artifacts exist and the hashes above were captured from `b4f1/`.

## Protected-path, ownership, and worktree audit

The protected audit relative to `c3aa53b` covered display/LVGL, touch, PSRAM,
USART3 MSP, PF3/PE10 output, and boot-stub paths:

```powershell
git diff c3aa53b -- Core/Src/ltdc.c App/Src/platform/lvgl_port.c `
  Core/Src/i2c.c App/Src/platform/touch_probe.c Core/Src/octospi.c `
  Drivers/BSP/Components/aps6408/aps6408.c `
  Drivers/BSP/wio_lite_ai/wio_lite_ai_ospi.c Core/Src/usart.c `
  App/Src/platform/msp_uart.c App/Src/platform/lighting_output_port.c boot_stub
git diff --name-only c3aa53b -- Middlewares/Third_Party/lvgl
git diff --check
```

Results: protected diff lines 0; tracked LVGL diffs 0; `git diff --check`
exit 0 with only the repository's LF-to-CRLF notices and no whitespace errors.

All pre-existing untracked evidence/build directories were preserved. The
current evidence-directory summary is:

```text
b4e6/  b4e7/  b4f1/
build-host-baseline/
build-host-ws4/  build-host-ws4-final/  build-host-ws4-final-fix/
build_ws4/  build_ws4_final/  build_ws4_task4_clean/
build_ws4_final_evidence2/  build_ws4_final_evidence3/
build_ws4_final_evidence4/  build_ws4_final_evidence5/
```

No evidence directory or LVGL content was deleted or rewritten.

## Remaining concerns and disposition

- Physical acceptance is still entirely `PENDING`; no build result is a
  waveform, wiring, current, thermal, identity, reset, or link-loss result.
- TIMPRE remains deferred as directed.
- The existing BSP unused-symbol and linker RWX warnings remain; this wave did
  not broaden scope to clean unrelated warnings.
- The legacy 10/30-pixel single-chain acceptance rows are now explicitly
  `DEPRECATED`; only the current 4/4/8/8 rows may be signed for this firmware.

Final disposition: software evidence complete; flash and physical acceptance
not performed.
