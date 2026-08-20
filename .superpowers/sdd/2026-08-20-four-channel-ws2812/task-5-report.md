# Task 5 Report: Four-channel WS2812 Wiring Documentation and Final Verification

## Status

Complete. The four-channel wiring, lighting checklist, and acceptance record
now describe four independent 4/4/8/8 WS2812 groups. This task made only
documentation/report changes and performed **no flash, no hardware connection,
and no hardware operation**. All four-channel physical checks remain
`PENDING`.

## Changed Documentation

- `docs/hardware/wiring.md`
  - Replaced the single-chain instructions with the exact independent routes:
    `PA0/D9 -> 100R -> 74AHCT125 1A/1Y -> 330R -> WS1 DIN (4 pixels)`,
    `PB3/D12 -> 100R -> 74AHCT125 2A/2Y -> 330R -> WS2 DIN (4 pixels)`,
    `PB4/MISO -> 100R -> 74AHCT125 3A/3Y -> 330R -> WS3 DIN (8 pixels)`, and
    `PB5/MOSI -> 100R -> 74AHCT125 4A/4Y -> 330R -> WS4 DIN (8 pixels)`.
  - Records protected 5 V, all 74AHCT125 OE pins low, 100 nF local
    decoupling, common ground, fused 5 V/3 A BEC, protected input bulk
    capacitance, optional 100 uF per remote group, 1 A software budget, and
    no 5 V feedback into STM32 pins.
- `docs/hardware/lighting-checklist.md`
  - Replaces one-chain acceptance wiring with four-channel wiring and adds
    explicit pending WS1/WS2 left/right identity and WS3/WS4 identity checks.
- `docs/hardware/acceptance-results.md`
  - Adds a current four-channel acceptance section. Oscilloscope, current,
    thermal, identity, reset-low, and link-loss work is explicitly `PENDING`.
- `docs/superpowers/reports/2026-08-20-four-channel-ws2812-report.md`
  - Records the verification evidence, hashes, protected-path audit, and
    no-flash disposition.

## Host Verification

Fresh build-directory commands run:

```powershell
cmake -S tests -B build-host-ws4-final
cmake --build build-host-ws4-final
ctest --test-dir build-host-ws4-final --output-on-failure
```

Results:

- Configure exited 0.
- Build exited 0.
- The exact CTest command exited 8 because the Visual Studio 16 2019
  multi-configuration generator received no `-C` value; all 12 tests reported
  `Not Run` with `Test not available without configuration. (Missing "-C <config>"?)`.

The required suite was then run in the built Debug configuration:

```powershell
ctest --test-dir build-host-ws4-final -C Debug --output-on-failure
```

Result: exit 0; **12/12 registered tests passed (100%)**.

## ARM Build and Artifact Verification

Fresh ARM build command:

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=build_ws4_final -j4
```

Result: exit 0. Fresh artifacts produced:

- `build_ws4_final/wio_ai.elf` — 6,972,824 bytes
- `build_ws4_final/wio_ai.hex` — 972,302 bytes
- `build_ws4_final/wio_ai.bin` — 345,652 bytes

Inspection commands:

```powershell
arm-none-eabi-objdump -h build_ws4_final/wio_ai.elf
arm-none-eabi-nm -u build_ws4_final/wio_ai.elf
Get-FileHash build_ws4_final/wio_ai.elf,build_ws4_final/wio_ai.hex,build_ws4_final/wio_ai.bin -Algorithm SHA256
```

Results:

- `objdump -h` exited 0; `.isr_vector` VMA/LMA is `0x08020000`.
- `nm -u` exited 0 with empty output (no unresolved symbols).
- SHA-256 hashes:

| Artifact | SHA-256 |
|---|---|
| `wio_ai.elf` | `6ABA173BEBD295D8005754DCACD85B0B2E03A60848101800FC12487790480319` |
| `wio_ai.hex` | `1B23B8D5D1BAF381A3B03FF1D33568688D261FD579A22F03F488C816707B99A7` |
| `wio_ai.bin` | `0ED77266A3FE850EACC4C4E247C500931D37EB8C6792D6CF977A077225FD092B` |

## Protected-path Audit

Commands:

```powershell
git diff c3aa53b -- Core/Src/ltdc.c App/Src/platform/lvgl_port.c App/Src/platform/msp_uart.c App/Src/platform/lighting_output_port.c boot_stub
git diff --check
git status --short
```

Results:

- The protected-path diff produced no output: no unintended change in display,
  LVGL, MSP UART, PF3/PE10 output, or boot-stub paths.
- `git diff --check` exited 0; only standard LF-to-CRLF notices appeared, with
  no whitespace errors.
- Evidence/build directories remain deliberately untracked and preserved:
  `build-host-baseline/`, `build-host-ws4/`, `build-host-ws4-final/`,
  `build_ws4/`, `build_ws4_final/`, and `build_ws4_task4_clean/`. LVGL was
  untouched.

## Commit

`55cf7c5 docs: record four-channel ws2812 acceptance plan`

## Concerns

- The exact CTest command in the task brief does not select a configuration
  under this Visual Studio generator; the transparently recorded `-C Debug`
  follow-up passed all 12 registered tests.
- Hardware evidence is intentionally absent. Waveform, reset-low, current,
  temperature, WS1/WS2 left/right identity, and stale/link-loss acceptance all
  remain `PENDING`; no build result is a physical-success claim.

## Fix Round 1: Fresh ARM Evidence

The original report did not connect a captured first-invocation exit status to
its artifact inspection. A newly created, unused short directory `b4e7/` was
used because long generated build-directory names can make the Windows GCC
link command exceed its process-launch limit.

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=b4e7 -j4
arm-none-eabi-objdump -h b4e7/wio_ai.elf
arm-none-eabi-nm -u b4e7/wio_ai.elf
Get-FileHash b4e7/wio_ai.elf,b4e7/wio_ai.hex,b4e7/wio_ai.bin -Algorithm SHA256
```

The captured first `make` invocation exited `0` (`b4e7/make.exit.txt`) and
produced ELF/HEX/BIN sizes 6,972,824 / 972,302 / 345,652 bytes. `objdump`
reports `.isr_vector` at `0x08020000`; `nm -u` is empty. SHA-256 hashes are
unchanged: ELF `6ABA173BEBD295D8005754DCACD85B0B2E03A60848101800FC12487790480319`,
HEX `1B23B8D5D1BAF381A3B03FF1D33568688D261FD579A22F03F488C816707B99A7`, and
BIN `0ED77266A3FE850EACC4C4E247C500931D37EB8C6792D6CF977A077225FD092B`.
No flash was performed; all physical acceptance remains `PENDING`.

## Final-Fix Round 2: Important Review Findings

Commit `605949a` closes the three Important software findings with explicit
TDD RED/GREEN evidence:

- Timing RED: the host link failed on missing `ws2812_compare_ticks`, and the
  timer manifest failed on missing target clock binding. GREEN: rounded
  350 ns/700 ns conversion proves 275 MHz -> 96/193 and production uses
  `ws2812_timer_clock_hz()`; invalid/out-of-period results fail safe.
- SPI3 retirement RED: the manifest failed because obsolete
  `Core/Src/spi.c` still existed. GREEN: the source/header, Make/CMake entries,
  SPI3 handle/MSP PB5 AF6/DMA1 Stream1/NVIC/IRQ path, and Cube SPI3/SPI3_TX
  metadata are removed. PB5 remains TIM3_CH2; real SPI1 display support and
  the generic HAL SPI driver remain.
- Busy-drop RED: host compilation failed because the distinct diagnostics
  field did not exist, and the application manifest found no observable
  accessor. GREEN: `busy_drop_count` increments only for non-IDLE rejection,
  including the protected race window. Invalid encoding/capacity, the normal
  34 ms limiter, and start/error paths remain separate and are tested.
  `ws2812_port_busy_drops()` feeds
  `DiagnosticsSnapshot.ws2812_busy_drops` through the application runtime
  diagnostics update.

Focused verification passed 3/3 (`unit_tests`, `lighting_app_wiring`, and
`ws2812_timer_manifest`). A genuinely fresh host directory,
`build-host-ws4-final-fix/`, configured and built successfully; the
**authoritative** command
`ctest --test-dir build-host-ws4-final-fix -C Debug --output-on-failure`
passed **12/12** tests.

A new short ARM directory, `b4f1/`, was absent before its first invocation:

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=b4f1 -j4
arm-none-eabi-objdump -h b4f1/wio_ai.elf
arm-none-eabi-nm -u b4f1/wio_ai.elf
Get-FileHash b4f1/wio_ai.elf,b4f1/wio_ai.hex,b4f1/wio_ai.bin -Algorithm SHA256
```

The first `make` exited 0 and produced ELF/HEX/BIN sizes
6,941,640 / 971,087 / 345,220 bytes. `.isr_vector` is `0x08020000` and
`nm -u` is empty. SHA-256: ELF
`A15FC711E336F547F3537C57675CAC4EF187D1EC451905C5440D55E131468D51`,
HEX `3B6FAFC9F84164873D05897BADD56E9E4A5D87C8CE1D2E01A0FAA8C05B43266A`,
BIN `005653E379B230CEE75279D84F4033B90AF2E15B6F4E6C8E9C3B6674030F6C8C`.

Protected display/LVGL, touch, PSRAM, USART3 MSP, PF3/PE10, and boot-stub
paths have zero diff lines relative to `c3aa53b`; tracked LVGL diffs are zero;
`git diff --check` exits 0. Existing evidence directories are preserved,
including `b4e6/`, `b4e7/`, `b4f1/`, `build-host-baseline/`,
`build-host-ws4*/`, `build_ws4*/`, and the four
`build_ws4_final_evidence2..5/` directories.

Acceptance documentation now spells `74AHCT125` in the current rows and marks
the legacy single-chain 10/30-pixel rows `DEPRECATED`; the only active pixel
scope is 4/4/8/8. TIMPRE remains deferred. No flash or hardware action was
performed, and every physical check remains `PENDING`.
