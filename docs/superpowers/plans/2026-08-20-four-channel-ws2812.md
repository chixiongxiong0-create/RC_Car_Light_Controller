# Four-Channel WS2812 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the single SPI3 WS2812 chain with four independently rendered outputs of 4, 4, 8, and 8 pixels using TIM2/TIM3 PWM DMA bursts.

**Architecture:** The lighting policy produces one fixed four-group RGB frame and applies the existing 1 A limiter across all 24 pixels. A pure encoder creates two interleaved CCR streams; a platform port atomically starts TIM2 CH1/CH2 and TIM3 CH1/CH2 and leaves all four pins low on completion or failure.

**Tech Stack:** C11, STM32H725 HAL TIM/DMA/DMAMUX, PA0/PB3/PB4/PB5 alternate functions, CMake/CTest host tests, GNU Arm Embedded 13.3, Make and STM32CubeIDE CMake manifests.

**Spec:** `docs/superpowers/specs/2026-08-20-four-channel-ws2812-design.md`

## Global Constraints

- WS1 and WS2 contain exactly 4 pixels each; WS3 and WS4 contain exactly 8 pixels each.
- The four output frames are independent and are submitted as one atomic physical frame.
- Use PA0/D9 AF1 TIM2_CH1, PB3/D12 AF1 TIM2_CH2, PB4/MISO AF2 TIM3_CH1, and PB5/MOSI AF2 TIM3_CH2.
- Use one update-triggered DMA burst per timer, beginning at CCR1 with a two-transfer burst.
- Serialize GRB, most-significant bit first, at approximately 800 kHz and append at least 64 zero-duty reset periods.
- Preserve the 1 A combined software current budget.
- Preserve PF3/D10, PE10/D11, display, touch, PSRAM, USART3 MSP, boot stub, and the application vector at `0x08020000`.
- Physical lights use only real, freshness-checked MSP state; demo presentation state never drives them.
- A malformed, short, stale, or lost lighting RC frame keeps the approved fail-safe behavior.
- Do not use blocking bit-banging and do not flash hardware during this implementation plan.

## File Structure

- Create `App/Inc/led/ws2812_frame.h`: fixed group counts and four-frame value type shared by policy, service, encoder, and port.
- Modify `App/Inc/lighting/lighting_controller.h` and `App/Src/lighting/lighting_controller.c`: render two rear clusters and two eight-pixel decorative strips into the new value type, then enforce one combined current budget.
- Modify `App/Inc/lighting/lighting_service.h` and `App/Src/lighting/lighting_service.c`: submit a complete `Ws2812Frame` atomically.
- Replace `App/Inc/platform/ws2812_port.h` and `App/Src/platform/ws2812_port.c`: pure paired-channel encoder plus dual-timer transport state and target HAL port.
- Modify `Core/Inc/tim.h`, `Core/Src/tim.c`, `Core/Inc/stm32h7xx_it.h`, `Core/Src/stm32h7xx_it.c`, and `Core/Src/stm32h7xx_hal_msp.c`: TIM2/TIM3, GPIO AF, DMAMUX, DMA IRQ, and safe-low hardware configuration.
- Modify `App/Src/app.c`: initialize and submit the new port and route timer DMA completion/error events.
- Modify `Makefile`, `cmake/st-project.cmake`, `wio_ai.ioc`, and `tests/CMakeLists.txt`: exact-once build and test manifests.
- Replace `tests/test_ws2812_encoder.c`; modify `tests/test_lighting_controller.c` and `tests/test_lighting_service.c`; create `tests/test_ws2812_timer_manifest.cmake`: focused behavior and wiring regression coverage.
- Modify `docs/hardware/wiring.md`, `docs/hardware/lighting-checklist.md`, and `docs/hardware/acceptance-results.md`: four-connector wiring and pending bench acceptance.

---

### Task 1: Four-Group Logical Lighting Frame

**Files:**
- Create: `App/Inc/led/ws2812_frame.h`
- Modify: `App/Inc/lighting/lighting_controller.h`
- Modify: `App/Src/lighting/lighting_controller.c`
- Modify: `App/Inc/lighting/lighting_service.h`
- Modify: `App/Src/lighting/lighting_service.c`
- Test: `tests/test_lighting_controller.c`
- Test: `tests/test_lighting_service.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `LedRgb`, the existing `VehicleState`, `led_estimated_ma`, and `led_limit_current`.
- Produces: `Ws2812Frame`, `ws2812_frame_clear`, and `LightingSubmitFn(uint32_t, const Ws2812Frame *, void *)`.

- [ ] **Step 1: Add failing group-layout and service tests**

Add assertions which require four independent arrays, fixed counts, and one complete-frame submit:

```c
static bool capture_groups(uint32_t now_ms, const Ws2812Frame *frame, void *ctx)
{
    Capture *capture = ctx;
    capture->now_ms = now_ms;
    capture->frame = *frame;
    capture->calls++;
    return true;
}

static void test_four_group_contract(void)
{
    assert(WS2812_GROUP_COUNT == 4u);
    assert(WS2812_GROUP_LENGTHS[0] == 4u);
    assert(WS2812_GROUP_LENGTHS[1] == 4u);
    assert(WS2812_GROUP_LENGTHS[2] == 8u);
    assert(WS2812_GROUP_LENGTHS[3] == 8u);
    assert(WS2812_TOTAL_PIXELS == 24u);
}
```

Extend controller tests to assert that a left turn changes WS1 and WS2 in
opposite physical orientation, and that a moving roof effect produces at least
one different RGB value between WS3 and WS4. Extend service tests to require
exactly one `capture_groups` call per tick and zero-valued groups before valid
lighting RC.

- [ ] **Step 2: Run the focused tests and observe RED**

Run:

```powershell
cmake -S tests -B build-host-ws4
cmake --build build-host-ws4 --target unit_tests
ctest --test-dir build-host-ws4 -R unit_tests --output-on-failure
```

Expected: compilation fails because `Ws2812Frame` and the new submit signature do not exist.

- [ ] **Step 3: Add the fixed four-group type**

Create `App/Inc/led/ws2812_frame.h` with the complete public contract:

```c
#pragma once
#include <stddef.h>
#include <string.h>
#include "led/led_controller.h"

enum {
    WS2812_GROUP_COUNT = 4,
    WS2812_GROUP_MAX_PIXELS = 8,
    WS2812_TOTAL_PIXELS = 24
};

static const size_t WS2812_GROUP_LENGTHS[WS2812_GROUP_COUNT] = {4u, 4u, 8u, 8u};

typedef struct {
    LedRgb groups[WS2812_GROUP_COUNT][WS2812_GROUP_MAX_PIXELS];
} Ws2812Frame;

static inline void ws2812_frame_clear(Ws2812Frame *frame)
{
    if (frame != NULL) {
        memset(frame, 0, sizeof *frame);
    }
}
```

- [ ] **Step 4: Render and limit all 24 logical pixels**

Replace `LightingFrame.pixels` with `Ws2812Frame ws2812`. Preserve the current
rear calculation as a four-pixel local array; copy it to WS1 and copy it in
reverse index order to WS2 so left/right indications match mirrored mounting.
Render the existing roof mode over one logical 16-pixel span and map indices
0..7 to WS3 and 8..15 to WS4. Flatten the four valid ranges into a local
24-pixel array, call `led_limit_current(flat, 24u, 1000u)`, copy the limited
values back, then set `estimated_ma = led_estimated_ma(flat, 24u)`.

Change the submit type and service call to:

```c
typedef bool (*LightingSubmitFn)(uint32_t now_ms,
                                 const Ws2812Frame *frame,
                                 void *ctx);

if (submit != NULL) {
    (void)submit(now_ms, &service->frame.ws2812, ctx);
}
```

Remove the `pixel_count` parameter from `lighting_controller_render` and
`lighting_service_tick`; fixed installed counts now come from the frame contract.

- [ ] **Step 5: Run focused and full host tests**

Run:

```powershell
cmake --build build-host-ws4 --target unit_tests
ctest --test-dir build-host-ws4 -R unit_tests --output-on-failure
ctest --test-dir build-host-ws4 --output-on-failure
```

Expected: all registered host tests pass; safety and current-limit regressions remain green.

- [ ] **Step 6: Commit the logical-frame change**

```powershell
git add App/Inc/led/ws2812_frame.h App/Inc/lighting App/Src/lighting tests/test_lighting_controller.c tests/test_lighting_service.c tests/CMakeLists.txt
git commit -m "refactor: model four ws2812 lighting groups"
```

### Task 2: Paired-Channel PWM Encoder and Atomic Transport State

**Files:**
- Replace: `App/Inc/platform/ws2812_port.h`
- Replace: `App/Src/platform/ws2812_port.c`
- Replace: `tests/test_ws2812_encoder.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `Ws2812Frame` from Task 1.
- Produces: `ws2812_encode_pair`, `Ws2812Transport`, and atomic dual-pair start/complete/error APIs used by Task 3.

- [ ] **Step 1: Replace SPI encoder tests with failing PWM-pair tests**

Define expected constants and verify GRB/MSB order, independence, reset slots,
capacity rejection, and guard words:

```c
enum { RESET_SLOTS = 64, DUTY_0 = 96, DUTY_1 = 193 };
uint32_t out[2u * (8u * 24u + RESET_SLOTS) + 2u];
for (size_t i = 0; i < sizeof out / sizeof out[0]; ++i) out[i] = 0xA55Au;

LedRgb a[4] = {{.g = 0x80u}};
LedRgb b[4] = {{.g = 0x00u}};
size_t slots = ws2812_encode_pair(a, b, 4u, DUTY_0, DUTY_1,
                                   RESET_SLOTS, out, 2u * (4u * 24u + RESET_SLOTS));
assert(slots == 4u * 24u + RESET_SLOTS);
assert(out[0] == DUTY_1);
assert(out[1] == DUTY_0);
assert(out[2u * slots] == 0xA55Au);
assert(out[2u * slots + 1u] == 0xA55Au);
```

Add a fake start callback for each timer pair and assert atomic semantics: both
starts occur only from idle; a failed second start invokes the supplied stop
callback for the first pair, increments `error_count`, and returns idle.

- [ ] **Step 2: Run the focused test and observe RED**

Run:

```powershell
cmake --build build-host-ws4 --target unit_tests
ctest --test-dir build-host-ws4 -R unit_tests --output-on-failure
```

Expected: compilation fails because `ws2812_encode_pair` and the new transport API are absent.

- [ ] **Step 3: Implement the pure paired encoder**

Expose this signature:

```c
size_t ws2812_encode_pair(const LedRgb *first, const LedRgb *second,
                          size_t pixel_count, uint32_t duty_0,
                          uint32_t duty_1, size_t reset_slots,
                          uint32_t *interleaved, size_t capacity_words);
```

For each pixel, emit G then R then B, bit 7 through bit 0. Write the first
channel duty followed by the second channel duty for every bit. Write two zero
words per reset slot. Return the number of PWM slots, not the number of words;
return zero for null inputs, zero pixels, insufficient capacity, reset shorter
than 64, or `duty_0 >= duty_1`.

- [ ] **Step 4: Implement transport state without HAL dependencies**

Use explicit callbacks so failure paths are host-testable:

```c
typedef bool (*Ws2812PairStartFn)(unsigned pair, const uint32_t *words,
                                  size_t slots, void *ctx);
typedef void (*Ws2812PairStopFn)(unsigned pair, void *ctx);

typedef struct {
    bool idle;
    uint8_t complete_mask;
    uint32_t last_submit_ms;
    uint32_t error_count;
} Ws2812Transport;
```

`ws2812_transport_submit` encodes both pairs before changing state, observes
the existing wrap-safe 34 ms rate limit, starts pair 0 then pair 1, and rolls
back pair 0 if pair 1 fails. Completion becomes idle only after bits 0 and 1
are both recorded. Any pair error calls stop for both pairs, increments the
counter once for the failed atomic frame, clears the completion mask, and
returns idle.

- [ ] **Step 5: Run focused and full host tests**

Run:

```powershell
cmake --build build-host-ws4 --target unit_tests
ctest --test-dir build-host-ws4 -R unit_tests --output-on-failure
ctest --test-dir build-host-ws4 --output-on-failure
```

Expected: all tests pass, including rollback, mixed completion order, and timer-wrap rate limiting.

- [ ] **Step 6: Commit the encoder and transport state**

```powershell
git add App/Inc/platform/ws2812_port.h App/Src/platform/ws2812_port.c tests/test_ws2812_encoder.c tests/CMakeLists.txt
git commit -m "feat: encode paired ws2812 pwm streams"
```

### Task 3: TIM2/TIM3 DMA Burst Hardware Port

**Files:**
- Modify: `App/Inc/platform/ws2812_port.h`
- Modify: `App/Src/platform/ws2812_port.c`
- Modify: `Core/Inc/tim.h`
- Replace: `Core/Src/tim.c`
- Modify: `Core/Inc/stm32h7xx_it.h`
- Modify: `Core/Src/stm32h7xx_it.c`
- Modify: `Core/Src/stm32h7xx_hal_msp.c`
- Modify: `wio_ai.ioc`
- Create: `tests/test_ws2812_timer_manifest.cmake`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: the pure transport and paired buffers from Task 2.
- Produces: `ws2812_port_init`, `ws2812_port_submit`, `ws2812_port_pair_complete`, and `ws2812_port_pair_error`.

- [ ] **Step 1: Add a failing target-wiring manifest**

Create a CMake script that reads the target sources and requires these literals:

```cmake
require_text("GPIO_AF1_TIM2")
require_text("GPIO_AF2_TIM3")
require_text("DMA_REQUEST_TIM2_UP")
require_text("DMA_REQUEST_TIM3_UP")
require_text("TIM_DMABASE_CCR1")
require_text("TIM_DMABURSTLENGTH_2TRANSFERS")
require_text("GPIO_PIN_0")
require_text("GPIO_PIN_3")
require_text("GPIO_PIN_4 | GPIO_PIN_5")
reject_text("HAL_SPI_Transmit_DMA")
```

Also require safe-low `HAL_GPIO_WritePin` calls before each corresponding
`HAL_GPIO_Init`, and register the script as `ws2812_timer_manifest` in CTest.

- [ ] **Step 2: Run the manifest and observe RED**

Run:

```powershell
cmake -S tests -B build-host-ws4
ctest --test-dir build-host-ws4 -R ws2812_timer_manifest --output-on-failure
```

Expected: failure because TIM3, timer update DMA, and the four AF outputs do not exist.

- [ ] **Step 3: Configure timers and safe GPIO alternate functions**

Define `htim2` and `htim3`. Derive `period`, `duty_0`, and `duty_1` from
`HAL_RCC_GetPCLK1Freq()` plus the APB1 timer-clock doubling rule; require the
computed period to fit 16 bits and produce a frequency within 790..810 kHz.
Configure PWM1 CH1/CH2 with initial pulse zero on both timers.

Before AF initialization, enable GPIOA/GPIOB clocks and write PA0, PB3, PB4,
and PB5 low. Configure PA0/PB3 with AF1 TIM2 and PB4/PB5 with AF2 TIM3,
push-pull, no pull, and very-high speed. Do not configure PA2/TIM2_CH3.

- [ ] **Step 4: Allocate two DMA streams and interrupts**

Use currently free `DMA1_Stream2` for `DMA_REQUEST_TIM2_UP` and
`DMA1_Stream3` for `DMA_REQUEST_TIM3_UP`, memory-to-peripheral, memory
increment, peripheral no-increment, word alignment, normal mode, high
priority. Link them to `htim2.hdma[TIM_DMA_ID_UPDATE]` and
`htim3.hdma[TIM_DMA_ID_UPDATE]`. Add both IRQ handlers and NVIC priority 5.

Leave DMA1 Stream0, DMA1 Stream1, DMA2 Stream1, USART3, LTDC, and OSPI
allocations unchanged. Mirror the selected pins, timers, and streams in
`wio_ai.ioc` so CubeMX metadata does not contradict the hand-maintained build.

- [ ] **Step 5: Start and stop each DMA burst safely**

In the target-only part of `ws2812_port.c`, keep two cache-line-aligned `uint32_t`
buffers sized for the 4-pixel and 8-pixel pairs. Clean exactly the cache-line
rounded encoded byte ranges. Start each pair with:

```c
HAL_TIM_DMABurst_MultiWriteStart(timer, TIM_DMABASE_CCR1,
    TIM_DMA_UPDATE, words, TIM_DMABURSTLENGTH_2TRANSFERS,
    (uint32_t)(slots * 2u));
```

Enable CH1 and CH2 only after the DMA burst is armed. Stop disables both
channels, stops the DMA burst, writes CCR1/CCR2 zero, and reconfigures the
four output pins low if initialization or start fails. Guard the HAL length
conversion and return false if it exceeds the API's accepted range.

- [ ] **Step 6: Route completion and error by timer identity**

Use the HAL DMA callbacks attached to each update DMA handle to call
`ws2812_port_pair_complete(0)` for TIM2 and `(1)` for TIM3. Error callbacks
call the matching error API. Do not route these transfers through SPI
callbacks. The port stop routine must make both timer pairs low before marking
the atomic transport idle.

- [ ] **Step 7: Run host manifests and build the ARM target**

Run:

```powershell
ctest --test-dir build-host-ws4 -R ws2812_timer_manifest --output-on-failure
ctest --test-dir build-host-ws4 --output-on-failure
make bsp_config_seedstudio=1 BUILD_DIR=build_ws4 -j4
```

Expected: manifest and all host tests pass; `build_ws4/wio_ai.elf`, `.hex`, and `.bin` exist.

- [ ] **Step 8: Commit the target transport**

```powershell
git add App/Inc/platform/ws2812_port.h App/Src/platform/ws2812_port.c Core/Inc/tim.h Core/Src/tim.c Core/Inc/stm32h7xx_it.h Core/Src/stm32h7xx_it.c Core/Src/stm32h7xx_hal_msp.c wio_ai.ioc tests/test_ws2812_timer_manifest.cmake tests/CMakeLists.txt
git commit -m "feat: drive four ws2812 outputs with timer dma"
```

### Task 4: Atomic Application Integration and SPI3 Retirement

**Files:**
- Modify: `App/Src/app.c`
- Modify: `Makefile`
- Modify: `cmake/st-project.cmake`
- Modify: `tests/test_lighting_app_wiring.cmake`
- Modify: `tests/test_ws2812_timer_manifest.cmake`

**Interfaces:**
- Consumes: `LightingSubmitFn` from Task 1 and target port from Task 3.
- Produces: one real-state-only, four-group physical submission per application lighting tick.

- [ ] **Step 1: Extend the application manifest and observe RED**

Require `submit_lighting` to accept `const Ws2812Frame *`, require exactly one
literal call to `ws2812_port_submit(now_ms, frame)`, and continue requiring
`lighting_tick(now_ms, real_state, low_battery)` while rejecting any call that
passes `presented`. Require that `HAL_SPI_TxCpltCallback`,
`HAL_SPI_ErrorCallback`, and `HAL_SPI_AbortCpltCallback` are absent from
`App/Src/app.c`.

Run:

```powershell
ctest --test-dir build-host-ws4 -R "lighting_app_wiring|ws2812_timer_manifest" --output-on-failure
```

Expected: failure because the old pixel/count submit and SPI callbacks remain.

- [ ] **Step 2: Switch the application to one four-group submission**

Implement:

```c
static bool submit_lighting(uint32_t now_ms,
                            const Ws2812Frame *frame,
                            void *ctx)
{
    (void)ctx;
    return ws2812_port_submit(now_ms, frame);
}
```

Keep `ws2812_port_init()` after diagnostics and real-state source
initialization. Remove `APP_LED_PIXEL_COUNT` from the service call. Preserve
the exact `real_state` data flow, PF3/PE10 apply call, diagnostic current, and
watchdog progress mark.

- [ ] **Step 3: Remove only obsolete SPI3 WS2812 wiring**

Remove the three application SPI callbacks and the `spi.h` include from
`app.c`. Remove SPI3 WS2812 initialization, DMA1 Stream1 IRQ dependency, and
SPI3-only target source entries only where inspection proves they have no
other compiled consumer. Keep `Core/Src/spi.c` in a build flavor if that
flavor still uses another SPI peripheral; the manifest must reject only the
obsolete `HAL_SPI_Transmit_DMA` WS2812 path.

- [ ] **Step 4: Verify exact-once Make and CMake source wiring**

Ensure the platform source remains listed exactly once in `Makefile` and
`cmake/st-project.cmake`. Run:

```powershell
ctest --test-dir build-host-ws4 -R "lighting_app_wiring|ws2812_timer_manifest|lvgl_make_manifest" --output-on-failure
ctest --test-dir build-host-ws4 --output-on-failure
make bsp_config_seedstudio=1 BUILD_DIR=build_ws4 -j4
```

Expected: all host tests pass and the ARM artifacts rebuild successfully.

- [ ] **Step 5: Commit application integration**

```powershell
git add App/Src/app.c Makefile cmake/st-project.cmake tests/test_lighting_app_wiring.cmake tests/test_ws2812_timer_manifest.cmake
git commit -m "refactor: wire atomic four-group lighting output"
```

### Task 5: Wiring Documentation and Final Verification

**Files:**
- Modify: `docs/hardware/wiring.md`
- Modify: `docs/hardware/lighting-checklist.md`
- Modify: `docs/hardware/acceptance-results.md`
- Create: `docs/superpowers/reports/2026-08-20-four-channel-ws2812-report.md`

**Interfaces:**
- Consumes: completed Tasks 1 through 4 and their test/build evidence.
- Produces: build-ready firmware artifacts and an explicit pending physical-acceptance checklist; performs no flash.

- [ ] **Step 1: Replace the one-chain wiring instructions**

Document the four data nets and connector order exactly:

```text
PA0/D9   -> 100R -> 74AHCT125 1A/1Y -> 330R -> WS1 DIN (4 pixels)
PB3/D12  -> 100R -> 74AHCT125 2A/2Y -> 330R -> WS2 DIN (4 pixels)
PB4/MISO -> 100R -> 74AHCT125 3A/3Y -> 330R -> WS3 DIN (8 pixels)
PB5/MOSI -> 100R -> 74AHCT125 4A/4Y -> 330R -> WS4 DIN (8 pixels)
```

Record 74AHCT125 at protected 5 V, all OE pins low, 100 nF local decoupling,
common ground, fused 5 V/3 A BEC, protected input bulk capacitance, optional
100 uF at each remote group, 1 A software budget, and no 5 V feedback into
STM32 pins. Mark every oscilloscope, current, thermal, left/right identity,
reset, and link-loss measurement as `PENDING`; do not claim physical success.

- [ ] **Step 2: Run a clean host suite**

Run:

```powershell
cmake -S tests -B build-host-ws4-final
cmake --build build-host-ws4-final
ctest --test-dir build-host-ws4-final --output-on-failure
```

Expected: 100% of registered tests pass. Record the exact pass count.

- [ ] **Step 3: Run a fresh ARM build and inspect artifacts**

Run:

```powershell
make bsp_config_seedstudio=1 BUILD_DIR=build_ws4_final -j4
arm-none-eabi-objdump -h build_ws4_final/wio_ai.elf
arm-none-eabi-nm -u build_ws4_final/wio_ai.elf
Get-FileHash build_ws4_final/wio_ai.elf,build_ws4_final/wio_ai.hex,build_ws4_final/wio_ai.bin -Algorithm SHA256
```

Expected: ELF/HEX/BIN exist; `.isr_vector` is at `0x08020000`; unresolved
symbol output is empty. Record all three SHA-256 hashes.

- [ ] **Step 4: Audit protected paths and write the report**

Run:

```powershell
git diff c3aa53b -- Core/Src/ltdc.c App/Src/platform/lvgl_port.c App/Src/platform/msp_uart.c App/Src/platform/lighting_output_port.c boot_stub
git diff --check
git status --short
```

Expected: no unintended changes in display, MSP UART, PF3/PE10 output, or boot
stub paths; no whitespace errors. Write the report with commands, exit status,
test count, vector address, unresolved-symbol result, hashes, protected-path
audit, no-flash statement, and all physical checks marked pending.

- [ ] **Step 5: Commit documentation and verification evidence**

```powershell
git add docs/hardware/wiring.md docs/hardware/lighting-checklist.md docs/hardware/acceptance-results.md docs/superpowers/reports/2026-08-20-four-channel-ws2812-report.md
git commit -m "docs: record four-channel ws2812 acceptance plan"
```

- [ ] **Step 6: Request final code review before integration**

Use `superpowers:requesting-code-review` against the complete change from
`c3aa53b` through the Task 5 commit. Resolve every Critical or Important
finding with a focused RED/GREEN test cycle, rerun the clean host suite and ARM
artifact checks, append the new evidence to the report, and commit each fix.
