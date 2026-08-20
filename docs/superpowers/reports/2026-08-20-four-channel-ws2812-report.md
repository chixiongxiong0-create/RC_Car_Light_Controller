# Four-channel WS2812 verification report — 2026-08-20

## Scope and hardware boundary

This report covers the four independent WS2812 outputs: WS1/WS2/WS3/WS4 at
4/4/8/8 pixels. The wiring and acceptance documents now require the specified
100 Ω MCU-input, 74AHCT125, and 330 Ω DIN topology for each output.

**No flash was performed. No hardware was connected or operated.** All
physical acceptance work remains `PENDING`, including oscilloscope waveform,
reset-low, current, thermal, left/right identity, and MSP link-loss checks.
Software results below are not physical-acceptance evidence.

## Host suite

Fresh directory commands:

```powershell
cmake -S tests -B build-host-ws4-final
cmake --build build-host-ws4-final
ctest --test-dir build-host-ws4-final --output-on-failure
```

The configure and build commands exited 0. With the Visual Studio 16 2019
multi-configuration generator, the exact third command exited 8 because it
selected no configuration: all 12 tests were reported `Not Run` with
`Test not available without configuration. (Missing "-C <config>"?)`. This is
a CTest invocation/configuration limitation, not a test failure; the build had
created the Debug executables.

Passing follow-up selection of that built configuration:

```powershell
ctest --test-dir build-host-ws4-final -C Debug --output-on-failure
```

Result: exit 0; **12/12 registered tests passed (100%)**.

## Fresh ARM build and artifacts

The fresh `build_ws4_final/` directory was created for the required target
build. The initial invocation completed the full source build and generated
the three artifacts; the follow-up invocation below exited 0 and reported
`Nothing to be done for 'all'.`

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=build_ws4_final -j4
```

Result: exit 0. Generated artifacts:

- `build_ws4_final/wio_ai.elf` — 6,972,824 bytes
- `build_ws4_final/wio_ai.hex` — 972,302 bytes
- `build_ws4_final/wio_ai.bin` — 345,652 bytes

Required artifact inspection commands:

```powershell
arm-none-eabi-objdump -h build_ws4_final/wio_ai.elf
arm-none-eabi-nm -u build_ws4_final/wio_ai.elf
Get-FileHash build_ws4_final/wio_ai.elf,build_ws4_final/wio_ai.hex,build_ws4_final/wio_ai.bin -Algorithm SHA256
```

Results:

- `objdump -h` exited 0 and reports `.isr_vector` VMA/LMA `0x08020000`.
- `nm -u` exited 0 with empty output: no unresolved symbols.
- SHA-256:

| Artifact | SHA-256 |
|---|---|
| `wio_ai.elf` | `6ABA173BEBD295D8005754DCACD85B0B2E03A60848101800FC12487790480319` |
| `wio_ai.hex` | `1B23B8D5D1BAF381A3B03FF1D33568688D261FD579A22F03F488C816707B99A7` |
| `wio_ai.bin` | `0ED77266A3FE850EACC4C4E247C500931D37EB8C6792D6CF977A077225FD092B` |

The target build emitted only pre-existing BSP unused-symbol warnings and the
pre-existing linker RWX LOAD-segment warning; it completed successfully.

## Protected-path and worktree audit

Commands:

```powershell
git diff c3aa53b -- Core/Src/ltdc.c App/Src/platform/lvgl_port.c App/Src/platform/msp_uart.c App/Src/platform/lighting_output_port.c boot_stub
git diff --check
git status --short
```

Results:

- The protected-path diff produced no output: no unintended display, LVGL,
  MSP UART, PF3/PE10 output-port, or boot-stub changes relative to `c3aa53b`.
- `git diff --check` exited 0. It printed only the repository's standard
  LF-to-CRLF notices for edited Markdown files; there were no whitespace
  errors.
- The intended tracked changes are only the three hardware documents and this
  report. Untracked evidence/build directories are deliberately preserved:
  `build-host-baseline/`, `build-host-ws4/`, `build-host-ws4-final/`,
  `build_ws4/`, `build_ws4_final/`, and `build_ws4_task4_clean/`. LVGL is
  untouched.

## Documentation commit

```powershell
git add docs/hardware/wiring.md docs/hardware/lighting-checklist.md docs/hardware/acceptance-results.md docs/superpowers/reports/2026-08-20-four-channel-ws2812-report.md
git commit -m "docs: record four-channel ws2812 acceptance plan"
```

Result: documentation/evidence commit created with the exact message above;
it contains no production-code changes and no hardware flash.

## Physical-acceptance disposition

`PENDING` — before any vehicle or permanent installation, perform and attach
real measurements for all four DIN outputs, data timing, reset low, 5 V/current
budget, thermal behavior, WS1/WS2 left/right identity, WS3/WS4 placement,
and stale/link-loss safe behavior. No present result may be interpreted as
physical success.
