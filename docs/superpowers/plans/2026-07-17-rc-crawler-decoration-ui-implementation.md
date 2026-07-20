# Wio Lite AI RC Crawler Decoration UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a 320×240 landscape, 30 FPS RC crawler decoration UI that reads INAV over read-only MSP UART and drives A/B/C/D screens plus 10–30 WS2812 LEDs without affecting vehicle control.

**Architecture:** Keep the existing STM32H725 HAL/BSP project bare-metal and non-blocking. USART3 becomes an exclusive MSP transport; decoded data flows through a hardware-independent `VehicleState` into LVGL screens and an LED policy engine. Pure logic is host-tested with CTest, while display, UART, SPI DMA, cache, and power behavior are verified on hardware.

**Tech Stack:** STM32H725 HAL/BSP, GNU Arm Embedded 13.3, C11, LVGL 9.5.0, LTDC RGB565, OSPI APS6408 PSRAM, DMA2D, MSP v1 with capability-gated INAV MSP2, SPI3 DMA WS2812 encoding, CMake/CTest host tests.

## Global Constraints

- Work only in `E:\workspace\fpv\wio_lite_ai_rc_ui`; never modify `E:\workspace\stm32\wio_lite_ai\wio_lite_ai`.
- Preserve the copied pre-existing `Core/Src/main.c` diff before correcting its stray backtick.
- UI board sends read-only MSP queries only; no RC override, configuration write, arming, servo, or ESC command.
- USART3 is 115200 baud, 8N1, 3.3V, non-inverted, full duplex, and exclusive to MSP; disable UART `printf` logging.
- Physical LCD remains 240×320 RGB565; application coordinates are 320×240 landscape.
- Target 30 FPS; RC/attitude-to-visual latency must remain below 100 ms.
- Camera, DCMI capture, AI network, Wi-Fi, BLE, audio, and MicroSD theme loading stay disabled.
- AUX selects A/B/C; button is fallback; touch is optional and must never block basic operation.
- Support 10–30 WS2812 pixels with a default 1A software current budget.
- Keep every loop task non-blocking; feed the watchdog only after all critical tasks make progress.
- Commit only files belonging to the current task; never include unrelated changes.

---

## File Structure

New application code lives under focused directories:

```text
App/Inc/app.h                         public application lifecycle
App/Src/app.c                         cooperative scheduler and wiring
App/Inc/app_time.h                    wrap-safe time helpers
App/Src/app_time.c
App/Inc/vehicle_state.h               normalized immutable UI input model
App/Src/vehicle_state.c
App/Inc/input_manager.h               AUX/button/touch arbitration
App/Src/input_manager.c
App/Inc/diagnostics.h                 counters and health snapshot
App/Src/diagnostics.c
App/Inc/msp/msp_codec.h               MSP v1/v2 framing only
App/Src/msp/msp_codec.c
App/Inc/msp/msp_client.h              polling schedule and response dispatch
App/Src/msp/msp_client.c
App/Inc/platform/msp_uart.h           USART3 IRQ ring-buffer adapter
App/Src/platform/msp_uart.c
App/Inc/platform/lvgl_port.h          LTDC/LVGL/cache integration
App/Src/platform/lvgl_port.c
App/Inc/platform/ws2812_port.h        SPI3 DMA byte transport
App/Src/platform/ws2812_port.c
App/Inc/led/led_controller.h          hardware-independent LED policy
App/Src/led/led_controller.c
App/Inc/ui/ui_app.h                    theme, status bar, page transitions
App/Src/ui/ui_app.c
App/Inc/ui/ui_theme.h
App/Src/ui/ui_theme.c
App/Inc/ui/screen_dashboard.h
App/Src/ui/screen_dashboard.c
App/Inc/ui/screen_face.h
App/Src/ui/screen_face.c
App/Inc/ui/screen_showcase.h
App/Src/ui/screen_showcase.c
App/Inc/platform/touch_probe.h         optional I2C identification only
App/Src/platform/touch_probe.c
tests/CMakeLists.txt                   native test target definitions
tests/test_*.c                         pure-logic tests
docs/hardware/*.md                     wiring and acceptance records
```

---

### Task 1: Protect the Baseline and Create a Buildable UI Application Mode

**Files:**
- Create: `docs/baseline/original-main-diff.patch`
- Create: `App/Inc/app.h`
- Create: `App/Src/app.c`
- Modify: `Core/Src/main.c:10-162,254-311`
- Modify: `Makefile:41-169,236-265,287-292`
- Modify: `cmake/st-project.cmake`

**Interfaces:**
- Consumes: existing HAL/BSP initialization and the copied user worktree.
- Produces: `void App_Init(void)`, `void App_Tick(uint32_t now_ms)`, and a build that does not initialize camera/AI.

- [ ] **Step 1: Capture and verify the copied user diff**

Run:

```powershell
git diff -- Core/Src/main.c | Set-Content -Encoding utf8 docs/baseline/original-main-diff.patch
git diff --check
```

Expected: patch records the `#endif`` change; `git diff --check` reports the stray backtick/syntax problem or no whitespace issue, but does not modify the source.

- [ ] **Step 2: Build the copied baseline to record the failure**

Run:

```powershell
make bsp_config_seedstudio=1 -j4
```

Expected: FAIL in `Core/Src/main.c` near line 307 because `#endif`` is not a valid directive.

- [ ] **Step 3: Add the minimal application lifecycle**

Create `App/Inc/app.h`:

```c
#pragma once
#include <stdint.h>

void App_Init(void);
void App_Tick(uint32_t now_ms);
```

Create `App/Src/app.c`:

```c
#include "app.h"

void App_Init(void) {}
void App_Tick(uint32_t now_ms) { (void)now_ms; }
```

- [ ] **Step 4: Convert `main.c` from camera/AI demo to UI app entrypoint**

Remove the stray backtick, remove calls to `Network_Init`, `STM32Ipl_InitLib`, `vittascience_i2c_init`, `BSP_CAMERA_ContinuousStart`, and `MX_X_CUBE_AI_Process`, and replace the infinite loop body with:

```c
#include "app.h"

/* after Hardware_Init(&App_Config); */
App_Init();

while (1) {
    App_Tick(HAL_GetTick());
}
```

In `Hardware_Init`, keep GPIO, OSPI RAM, CRC, OCTOSPI2, LTDC, LEDs, and button. Remove TIM2 camera clock and camera initialization. Do not call `BSP_COM_Init`, because USART3 will belong to MSP in Task 4.

- [ ] **Step 5: Remove inactive camera/AI code from the link, without deleting source files**

Change the Makefile lists so `C_SOURCES_COMMON` excludes `X-CUBE-AI/App/*`, OV2640, DCMI driver, STM32 IPL sources, `Core/Src/dcmi.c`, and `Core/Src/vittascience_i2c.c`; add:

```make
C_SOURCES_APP = \
App/Src/app.c

C_SOURCES = $(C_SOURCES_COMMON) $(C_SOURCES_APP) $(C_SOURCES_SEEDSTUDIO_SCREEN)
C_INCLUDES += -IApp/Inc
LIBS = -lc -lm -lnosys
```

Mirror the same source/include removal and `App/Src/app.c` addition in `cmake/st-project.cmake`.

- [ ] **Step 6: Build and inspect memory**

Run:

```powershell
make bsp_config_seedstudio=1 -j4
& 'D:\development\arm_tools\arm_tools\13.3_rel1\bin\arm-none-eabi-size.exe' build_seed/wio_ai.elf
```

Expected: PASS; `wio_ai.elf`, `.hex`, and `.bin` exist; no unresolved AI/camera symbol remains.

- [ ] **Step 7: Commit**

```powershell
git add docs/baseline App Core/Src/main.c Makefile cmake/st-project.cmake
git commit -m "refactor: create isolated decoration UI app mode"
```

---

### Task 2: Add Native Test Harness and Wrap-Safe Time Utilities

**Files:**
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_main.c`
- Create: `tests/test_app_time.c`
- Create: `App/Inc/app_time.h`
- Create: `App/Src/app_time.c`

**Interfaces:**
- Consumes: standard C11 only.
- Produces: `bool time_reached(uint32_t now, uint32_t deadline)` and `uint32_t elapsed_ms(uint32_t now, uint32_t then)`.

- [ ] **Step 1: Write failing wrap-around tests**

`tests/test_app_time.c`:

```c
#include <assert.h>
#include "app_time.h"

void test_app_time(void) {
    assert(elapsed_ms(25u, 10u) == 15u);
    assert(elapsed_ms(5u, 0xfffffff0u) == 21u);
    assert(time_reached(100u, 100u));
    assert(!time_reached(99u, 100u));
    assert(time_reached(5u, 0xfffffff0u));
}
```

`tests/test_main.c` calls `test_app_time()` and prints `all tests passed`.

- [ ] **Step 2: Configure and run the failing native test**

`tests/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(wio_ui_tests C)
set(CMAKE_C_STANDARD 11)
enable_testing()
add_executable(unit_tests test_main.c test_app_time.c ../App/Src/app_time.c)
target_include_directories(unit_tests PRIVATE ../App/Inc)
add_test(NAME unit_tests COMMAND unit_tests)
```

Run:

```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

Expected: FAIL because `app_time.h` does not exist.

- [ ] **Step 3: Implement wrap-safe helpers**

```c
/* App/Inc/app_time.h */
#pragma once
#include <stdbool.h>
#include <stdint.h>
uint32_t elapsed_ms(uint32_t now, uint32_t then);
bool time_reached(uint32_t now, uint32_t deadline);

/* App/Src/app_time.c */
#include "app_time.h"
uint32_t elapsed_ms(uint32_t now, uint32_t then) { return now - then; }
bool time_reached(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}
```

- [ ] **Step 4: Run host and embedded builds**

Run:

```powershell
cmake --build build-host
ctest --test-dir build-host --output-on-failure
make bsp_config_seedstudio=1 -j4
```

Expected: `100% tests passed`; embedded build passes after adding `App/Src/app_time.c` to both build systems.

- [ ] **Step 5: Commit**

```powershell
git add tests App/Inc/app_time.h App/Src/app_time.c Makefile cmake/st-project.cmake
git commit -m "test: add native harness and wrap-safe time"
```

---

### Task 3: Implement MSP Codec and Read-Only Poll Scheduler

**Files:**
- Create: `App/Inc/msp/msp_codec.h`
- Create: `App/Src/msp/msp_codec.c`
- Create: `App/Inc/msp/msp_client.h`
- Create: `App/Src/msp/msp_client.c`
- Create: `tests/test_msp_codec.c`
- Create: `tests/test_msp_client.c`
- Modify: `tests/test_main.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `elapsed_ms()` and byte-oriented transport callbacks.
- Produces: `msp_codec_feed()`, `msp_v1_encode_request()`, `msp_client_tick()`, and validated `MspFrame` callbacks.

- [ ] **Step 1: Define exact codec API and failing tests**

`msp_codec.h`:

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MSP_MAX_PAYLOAD 64u
typedef struct { uint16_t command; uint8_t length; uint8_t payload[MSP_MAX_PAYLOAD]; } MspFrame;
typedef struct { uint8_t state, length, command, offset, checksum; MspFrame frame; } MspCodec;

void msp_codec_reset(MspCodec *codec);
bool msp_codec_feed(MspCodec *codec, uint8_t byte, MspFrame *out);
size_t msp_v1_encode_request(uint8_t command, uint8_t out[6]);
```

Test exact request bytes and response parsing:

```c
uint8_t request[6];
assert(msp_v1_encode_request(105, request) == 6);
assert(memcmp(request, (uint8_t[]){'$', 'M', '<', 0, 105, 105}, 6) == 0);

MspCodec codec = {0}; MspFrame frame;
const uint8_t response[] = {'$','M','>',2,108,0x10,0x20,(uint8_t)(2^108^0x10^0x20)};
for (size_t i = 0; i < sizeof response - 1; ++i) assert(!msp_codec_feed(&codec, response[i], &frame));
assert(msp_codec_feed(&codec, response[sizeof response - 1], &frame));
assert(frame.command == 108 && frame.length == 2 && frame.payload[1] == 0x20);
```

Also test bad checksum, oversize length, noise before `$`, and recovery on the next valid frame.

- [ ] **Step 2: Run tests and verify failure**

Run `cmake --build build-host && ctest --test-dir build-host --output-on-failure`.

Expected: FAIL because codec functions are undefined.

- [ ] **Step 3: Implement the byte state machine**

Implement states `WAIT_DOLLAR`, `WAIT_M`, `WAIT_DIRECTION`, `WAIT_LENGTH`, `WAIT_COMMAND`, `WAIT_PAYLOAD`, and `WAIT_CHECKSUM`. XOR length, command, and payload; emit only `$M>` frames with valid checksum and payload length no greater than 64.

Core completion logic:

```c
if (codec->state == WAIT_CHECKSUM) {
    const bool valid = byte == codec->checksum;
    if (valid) *out = codec->frame;
    msp_codec_reset(codec);
    return valid;
}
```

- [ ] **Step 4: Define polling API and failing schedule tests**

`msp_client.h`:

```c
typedef bool (*MspWriteFn)(const uint8_t *data, size_t length, void *ctx);
typedef void (*MspFrameFn)(const MspFrame *frame, uint32_t now_ms, void *ctx);
typedef struct {
    MspCodec codec;
    MspWriteFn write;
    MspFrameFn on_frame;
    void *ctx;
    uint32_t sent_at_ms, last_valid_ms;
    uint32_t due_ms[5];
    uint8_t active_query;
    bool awaiting;
    uint32_t requests, timeouts, checksum_errors;
} MspClient;

void msp_client_init(MspClient *, MspWriteFn, MspFrameFn, void *ctx);
void msp_client_rx_byte(MspClient *, uint8_t byte, uint32_t now_ms);
void msp_client_tick(MspClient *, uint32_t now_ms);
```

Test that only one request is in flight, timeout occurs after 40 ms, and independent deadlines send RC every 40 ms, attitude every 50 ms, analog/GPS every 200 ms, and status every 500 ms. Over a 2000 ms simulated run, allow one-period jitter when another request is in flight but assert no command is sent faster than its configured period.

- [ ] **Step 5: Implement a fixed read-only request table**

Use these common command IDs and query descriptors:

```c
enum { MSP_API_VERSION=1, MSP_FC_VARIANT=2, MSP_FC_VERSION=3,
       MSP_STATUS=101, MSP_RAW_GPS=106, MSP_ATTITUDE=108,
       MSP_ANALOG=110, MSP_RC=105 };

typedef struct { uint8_t command; uint16_t period_ms; } QueryDef;
static const QueryDef queries[] = {
    { MSP_RC, 40 }, { MSP_ATTITUDE, 50 }, { MSP_ANALOG, 200 },
    { MSP_RAW_GPS, 200 }, { MSP_STATUS, 500 }
};
```

When no request is active, choose the due query with the largest positive lateness, send it, and advance only that query's deadline by its period. Wait up to 40 ms for the response. Query API/variant/version once before entering the schedule. Never expose a generic write-command API.

- [ ] **Step 6: Run all tests and embedded build**

Expected: all native tests pass; embedded image links after adding four MSP files and include paths.

- [ ] **Step 7: Commit**

```powershell
git add App/Inc/msp App/Src/msp tests Makefile cmake/st-project.cmake
git commit -m "feat: add read-only MSP client"
```

---

### Task 4: Decode MSP Into VehicleState With Stale/Link-Loss Semantics

**Files:**
- Create: `App/Inc/vehicle_state.h`
- Create: `App/Src/vehicle_state.c`
- Create: `tests/test_vehicle_state.c`
- Modify: `tests/test_main.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: validated `MspFrame` and millisecond timestamps.
- Produces: `vehicle_state_on_msp()`, `vehicle_state_tick()`, and `const VehicleState *vehicle_state_get()`.

- [ ] **Step 1: Define normalized state and failing decode tests**

```c
typedef enum { LINK_STARTING, LINK_OK, LINK_STALE, LINK_LOST } LinkState;
typedef struct {
    float throttle;        /* -1.0 reverse to +1.0 forward */
    float steering;        /* -1.0 left to +1.0 right */
    float aux_page;        /* -1.0, 0.0, +1.0 */
    float roll_deg, pitch_deg, heading_deg;
    float battery_v;
    uint16_t rssi;
    uint8_t gps_sats;
    bool armed;
    LinkState link;
    uint32_t last_msp_ms, last_rc_ms, last_attitude_ms;
} VehicleState;

void vehicle_state_init(void);
bool vehicle_state_on_msp(const MspFrame *frame, uint32_t now_ms);
void vehicle_state_tick(uint32_t now_ms);
const VehicleState *vehicle_state_get(void);
```

Tests feed little-endian `MSP_RC`, `MSP_ATTITUDE`, `MSP_ANALOG`, `MSP_RAW_GPS`, and `MSP_STATUS` payloads. Assert 1000/1500/2000 μs map to -1/0/+1, attitude uses INAV units, malformed lengths are rejected, 500 ms becomes `LINK_STALE`, 2000 ms becomes `LINK_LOST`, and 500 ms of valid recovery becomes `LINK_OK`.

- [ ] **Step 2: Run failing tests**

Expected: FAIL because state decoder is absent.

- [ ] **Step 3: Implement bounded little-endian readers and filters**

Use local helpers that check `offset + width <= frame->length`. Normalize channels with clamp and a 0.03 deadband. Apply an exponential filter only to live RC/attitude values:

```c
static float lowpass(float previous, float input, float alpha) {
    return previous + alpha * (input - previous);
}
```

Use `alpha=0.35f` for RC and `0.25f` for attitude. Do not update any field if the command payload length is invalid.

- [ ] **Step 4: Implement recovery hysteresis**

Track `recovery_started_ms`; after loss, require valid frames for 500 consecutive milliseconds before setting `LINK_OK`. Fast values remain frozen while stale/lost.

- [ ] **Step 5: Run tests and embedded build**

Expected: all tests and firmware build pass.

- [ ] **Step 6: Commit**

```powershell
git add App/Inc/vehicle_state.h App/Src/vehicle_state.c tests Makefile cmake/st-project.cmake
git commit -m "feat: decode MSP vehicle state"
```

---

### Task 5: Integrate Exclusive USART3 MSP Transport and Cooperative Scheduler

**Files:**
- Create: `App/Inc/platform/msp_uart.h`
- Create: `App/Src/platform/msp_uart.c`
- Create: `App/Inc/diagnostics.h`
- Create: `App/Src/diagnostics.c`
- Modify: `Core/Src/usart.c`
- Modify: `Core/Src/stm32h7xx_it.c`
- Modify: `Core/Inc/stm32h7xx_it.h`
- Modify: `App/Src/app.c`
- Modify: `Makefile`
- Modify: `cmake/st-project.cmake`

**Interfaces:**
- Consumes: `huart3`, `MspClient`, and `vehicle_state_on_msp()`.
- Produces: non-blocking `msp_uart_init()`, `msp_uart_read()`, `msp_uart_write()`, and scheduler health counters.

- [ ] **Step 1: Define transport interface and ring buffer invariants**

```c
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
void msp_uart_init(void);
bool msp_uart_read(uint8_t *byte);
bool msp_uart_write(const uint8_t *data, size_t length);
uint32_t msp_uart_overruns(void);
```

Use a power-of-two 256-byte RX ring. ISR advances `head`; application advances `tail`; on full buffer increment overrun and discard the newest byte.

- [ ] **Step 2: Configure USART3 and interrupts**

Call `MX_USART3_UART_Init()` from app hardware setup. Enable `UART_IT_RXNE`, add `USART3_IRQHandler()` that calls `HAL_UART_IRQHandler(&huart3)`, and re-arm one-byte `HAL_UART_Receive_IT` in `HAL_UART_RxCpltCallback`.

Do not initialize `BSP_COM` and do not use `printf` after USART3 ownership transfers to MSP. Retain SWD for debugging.

- [ ] **Step 3: Implement bounded TX**

Because MSP requests are six bytes, use a non-blocking busy guard around `HAL_UART_Transmit_IT`; reject a write while TX is active. Clear the guard in `HAL_UART_TxCpltCallback`.

- [ ] **Step 4: Wire the app scheduler**

`App_Tick()` must remain short:

```c
static void on_msp_frame(const MspFrame *frame, uint32_t now_ms, void *ctx) {
    (void)ctx;
    (void)vehicle_state_on_msp(frame, now_ms);
}

void App_Tick(uint32_t now_ms) {
    uint8_t byte;
    while (msp_uart_read(&byte)) msp_client_rx_byte(&client, byte, now_ms);
    msp_client_tick(&client, now_ms);
    vehicle_state_tick(now_ms);
    input_manager_tick(now_ms, vehicle_state_get());
    ui_app_tick(now_ms, vehicle_state_get());
    led_controller_tick(now_ms, vehicle_state_get());
    diagnostics_tick(now_ms);
}
```

Temporarily provide no-op input/UI/LED adapters until their tasks land; each no-op must have the final signature.

- [ ] **Step 5: Hardware smoke test USART3**

Connect INAV TX→PD9, INAV RX←PD8, and GND. Configure the INAV port for MSP at 115200. Flash, then inspect `MspClient.last_valid_ms` and `VehicleState` through the debugger.

Expected: counters increase, valid frames arrive, and moving steering/throttle changes normalized values; INAV control remains unaffected.

- [ ] **Step 6: Commit**

```powershell
git add App Core/Src/usart.c Core/Src/stm32h7xx_it.c Core/Inc/stm32h7xx_it.h Makefile cmake/st-project.cmake
git commit -m "feat: connect INAV MSP over USART3"
```

---

### Task 6: Vendor LVGL 9.5.0 and Bring Up 320×240 LTDC Rendering

**Files:**
- Create: `.gitmodules`
- Create submodule: `Middlewares/Third_Party/lvgl`
- Create: `lv_conf.h`
- Create: `App/Inc/platform/lvgl_port.h`
- Create: `App/Src/platform/lvgl_port.c`
- Create: `App/Inc/ui/ui_app.h`
- Create: `App/Src/ui/ui_app.c`
- Modify: `App/Src/app.c`
- Modify: `Core/Src/ltdc.c`
- Modify: `STM32H725AEIX_PSRAM.ld`
- Modify: `Makefile`
- Modify: `cmake/st-project.cmake`

**Interfaces:**
- Consumes: LTDC, PSRAM memory-mapped mode, HAL tick, and cache maintenance.
- Produces: `lvgl_port_init()`, `lvgl_port_tick()`, `ui_app_init()`, `ui_app_tick()`.

- [ ] **Step 1: Add pinned LVGL source**

Run:

```powershell
git submodule add https://github.com/lvgl/lvgl.git Middlewares/Third_Party/lvgl
git -C Middlewares/Third_Party/lvgl checkout v9.5.0
git add .gitmodules Middlewares/Third_Party/lvgl
```

Expected: submodule HEAD identifies tag `v9.5.0`.

- [ ] **Step 2: Configure only required LVGL features**

Create `lv_conf.h` from the v9.5.0 template with these explicit values:

```c
#define LV_COLOR_DEPTH 16
#define LV_USE_OS 0
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (96U * 1024U)
#define LV_DEF_REFR_PERIOD 33
#define LV_DPI_DEF 130
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_LABEL 1
#define LV_USE_IMAGE 1
#define LV_USE_ANIMIMG 1
```

- [ ] **Step 3: Allocate one physical scanout buffer and one LVGL landscape tile buffer**

Add linker sections with 32-byte alignment:

```ld
.ltdc_scanout (NOLOAD) : { . = ALIGN(32); *(.ltdc_scanout) } > PSRAM_BUF
.lvgl_draw (NOLOAD) : { . = ALIGN(32); *(.lvgl_draw) } > PSRAM_BUF
```

Declare:

```c
#define UI_WIDTH 320u
#define UI_HEIGHT 240u
#define PHYSICAL_WIDTH 240u
#define PHYSICAL_HEIGHT 320u
#define DRAW_LINES 24u
__attribute__((section(".ltdc_scanout"), aligned(32)))
static uint16_t scanout[PHYSICAL_WIDTH * PHYSICAL_HEIGHT];
__attribute__((section(".lvgl_draw"), aligned(32)))
static uint16_t draw_buf[UI_WIDTH * DRAW_LINES];
```

- [ ] **Step 4: Register the LVGL display**

```c
void lvgl_port_init(void) {
    lv_init();
    display = lv_display_create(UI_WIDTH, UI_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buf, NULL, sizeof draw_buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush_cb);
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)scanout, 0);
}
```

`flush_cb` rotates each landscape pixel into the physical portrait scanout using the fixed clockwise mapping `physical_x = logical_y`, `physical_y = 319 - logical_x`:

```c
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint16_t *src = (const uint16_t *)px_map;
    for (int32_t y = area->y1; y <= area->y2; ++y) {
        for (int32_t x = area->x1; x <= area->x2; ++x) {
            const uint32_t physical_x = (uint32_t)y;
            const uint32_t physical_y = 319u - (uint32_t)x;
            scanout[physical_y * PHYSICAL_WIDTH + physical_x] = *src++;
        }
    }
    clean_scanout_cache_for_rotated_area(area);
    lv_display_flush_ready(disp);
}
```

Keep LTDC configured for its existing 240×320 physical timing. `clean_scanout_cache_for_rotated_area()` rounds start/end addresses to 32-byte cache lines and calls `SCB_CleanDCache_by_Addr`. Do not change page coordinates or allocate camera buffers.

- [ ] **Step 5: Add a deterministic diagnostic screen**

Create a dark background with yellow border, `WIO TRAIL SYSTEM`, a 0–59 FPS label, and a moving 20 px yellow block. Call `lv_timer_handler()` from `lvgl_port_tick()` and `lv_tick_inc(delta_ms)` with wrap-safe delta.

- [ ] **Step 6: Build and hardware-verify display**

Expected: landscape content is not mirrored, colors are correct, no tearing is visible, and debugger-measured average is at least 28 FPS over 60 seconds.

- [ ] **Step 7: Commit**

```powershell
git add .gitmodules Middlewares/Third_Party/lvgl lv_conf.h App Core/Src/ltdc.c STM32H725AEIX_PSRAM.ld Makefile cmake/st-project.cmake
git commit -m "feat: bring up LVGL landscape display"
```

---

### Task 7: Implement Theme, Input Arbitration, and Dashboard A

**Files:**
- Create: `App/Inc/input_manager.h`
- Create: `App/Src/input_manager.c`
- Create: `App/Inc/ui/ui_theme.h`
- Create: `App/Src/ui/ui_theme.c`
- Create: `App/Inc/ui/screen_dashboard.h`
- Create: `App/Src/ui/screen_dashboard.c`
- Create: `tests/test_input_manager.c`
- Modify: `App/Src/ui/ui_app.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `VehicleState`, button events, optional touch page requests.
- Produces: `UiPage input_manager_page(void)` and dashboard create/update functions.

- [ ] **Step 1: Write failing input-priority tests**

```c
typedef enum { UI_PAGE_DASHBOARD, UI_PAGE_FACE, UI_PAGE_SHOWCASE } UiPage;
void input_manager_init(void);
void input_manager_set_button(bool pressed, uint32_t now_ms);
void input_manager_set_touch_page(UiPage page);
void input_manager_tick(uint32_t now_ms, const VehicleState *state);
UiPage input_manager_page(void);
```

Assert AUX low/mid/high maps A/B/C; a 30–800 ms button press cycles; a ≥1000 ms press requests brightness mode; touch is ignored when unavailable; any AUX zone change overrides the local page.

- [ ] **Step 2: Implement hysteretic AUX zones and debounced button state machine**

Use thresholds `-0.35f` and `+0.35f`, with 0.08 hysteresis. Debounce button for 30 ms. Keep page and brightness request as explicit state, not LVGL callbacks.

- [ ] **Step 3: Build the shared rugged theme**

Define colors:

```c
#define UI_COLOR_BG       lv_color_hex(0x111820)
#define UI_COLOR_PANEL    lv_color_hex(0x1C2932)
#define UI_COLOR_YELLOW   lv_color_hex(0xFFB000)
#define UI_COLOR_GREEN    lv_color_hex(0x41D18B)
#define UI_COLOR_BLUE     lv_color_hex(0x5FB9FF)
#define UI_COLOR_MUTED    lv_color_hex(0x93A7B1)
```

Provide reusable panel, value, caption, warning, and top-status styles. Avoid shadows and large alpha layers that threaten 30 FPS.

- [ ] **Step 4: Implement Dashboard A**

Expose:

```c
lv_obj_t *screen_dashboard_create(void);
void screen_dashboard_update(const VehicleState *state);
```

Layout: 128 px left throttle arc/value, direction and steering below; four right-side tiles for battery, pitch, roll, GPS; top bar for armed/mode/link. Update labels only when formatted text changes. Animate arc/value using the filtered `VehicleState`, not raw MSP.

- [ ] **Step 5: Verify tests, build, and hardware behavior**

Expected: host tests pass; AUX selects A; live throttle/steering/attitude update within 100 ms; 60-second average remains ≥28 FPS.

- [ ] **Step 6: Commit**

```powershell
git add App tests Makefile cmake/st-project.cmake
git commit -m "feat: add rugged dashboard and input arbitration"
```

---

### Task 8: Implement Mechanical Face B and Link/Error Expressions

**Files:**
- Create: `App/Inc/ui/screen_face.h`
- Create: `App/Src/ui/screen_face.c`
- Create: `tests/test_face_model.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `App/Src/ui/ui_app.c`

**Interfaces:**
- Consumes: `VehicleState` and UI link/low-battery flags.
- Produces: pure `FaceModel face_model_from_state()` plus LVGL face create/update functions.

- [ ] **Step 1: Write failing expression-model tests**

```c
typedef enum { FACE_IDLE, FACE_FOCUSED, FACE_REVERSE, FACE_LOW_BATTERY, FACE_LINK_LOST } FaceMood;
typedef struct { FaceMood mood; int16_t gaze_x; uint8_t aperture; } FaceModel;
FaceModel face_model_from_state(const VehicleState *state, bool low_battery);
```

Assert lost link overrides low battery, low battery overrides reverse, reverse overrides focused, steering maps gaze to -24…24 px, and absolute throttle maps aperture to 35…100%.

- [ ] **Step 2: Implement pure priority/model function**

Keep it independent of LVGL so all expression logic is host-tested.

- [ ] **Step 3: Implement LVGL mechanical eyes**

Create two clipped polygon-like eye containers from rectangles with transform angles; use yellow for normal, white for reverse, pulsing yellow for low battery, and alternating yellow/off for link loss. Update at 30 Hz without deleting/recreating objects.

- [ ] **Step 4: Verify and commit**

Expected: tests pass; transitions are smooth; B stays ≥28 FPS.

```powershell
git add App tests Makefile cmake/st-project.cmake
git commit -m "feat: add mechanical expression screen"
```

---

### Task 9: Implement Showcase C and Unified D Page Transitions

**Files:**
- Create: `App/Inc/ui/screen_showcase.h`
- Create: `App/Src/ui/screen_showcase.c`
- Create: `tests/test_showcase_model.c`
- Modify: `App/Src/ui/ui_app.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: selected `UiPage`, `VehicleState`, and monotonic time.
- Produces: deterministic showcase scenes and A/B/C transition controller.

- [ ] **Step 1: Write failing deterministic scene tests**

Define scenes `SHOW_LOGO`, `SHOW_SLOGAN`, `SHOW_ARMED`, `SHOW_MOTION`. Assert idle scenes rotate every 4000 ms; arming holds `SHOW_ARMED` for 1500 ms; throttle magnitude >0.2 selects motion; steering sign controls stripe direction.

- [ ] **Step 2: Implement the pure showcase model**

Expose `ShowcaseModel showcase_model_update(previous, state, now_ms)` and test tick wrap-around.

- [ ] **Step 3: Implement C visuals**

Use built-in text and vector primitives only: `WIO TRAIL SYSTEM`, `CRAWLER H725`, `BUILT FOR SLOW LINES AND HARD CLIMBS`, diagonal warning stripe, and bottom status row. Do not add SD or network assets.

- [ ] **Step 4: Implement D transition state machine**

On page change, animate a yellow/black shutter across the screen for 100 ms, swap the hidden page, then retract for 100 ms. AUX selection always wins over button/touch. Keep all three screens allocated to avoid heap churn.

- [ ] **Step 5: Verify and commit**

Expected: model tests pass; AUX changes complete within 250 ms; no transient blank frame.

```powershell
git add App tests Makefile cmake/st-project.cmake
git commit -m "feat: unify dashboard face and showcase modes"
```

---

### Task 10: Implement WS2812 Policy and SPI3 DMA Output

**Files:**
- Create: `App/Inc/led/led_controller.h`
- Create: `App/Src/led/led_controller.c`
- Create: `App/Inc/platform/ws2812_port.h`
- Create: `App/Src/platform/ws2812_port.c`
- Create: `tests/test_led_controller.c`
- Create: `Core/Inc/spi.h`
- Create: `Core/Src/spi.c`
- Modify: `Core/Src/stm32h7xx_it.c`
- Modify: `Core/Src/stm32h7xx_hal_msp.c`
- Modify: `wio_ai.ioc`
- Modify: `App/Src/app.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `VehicleState`, page, low-battery/fault flags, and a 10–30 pixel configured count.
- Produces: RGB policy frame plus non-blocking SPI3 DMA transfer on Arduino-header PB5 MOSI.

- [ ] **Step 1: Write failing policy and current-limit tests**

```c
#define LED_MAX_PIXELS 30u
typedef struct { uint8_t r, g, b; } LedRgb;
typedef enum { LED_NORMAL, LED_REVERSE, LED_TURN_LEFT, LED_TURN_RIGHT,
               LED_LOW_BATTERY, LED_LINK_LOST, LED_BOARD_FAULT } LedMode;
void led_controller_render(uint32_t now_ms, const VehicleState *, UiPage,
                           bool low_battery, bool board_fault,
                           LedRgb out[LED_MAX_PIXELS], size_t count);
uint32_t led_estimated_ma(const LedRgb *pixels, size_t count);
void led_limit_current(LedRgb *pixels, size_t count, uint32_t budget_ma);
```

Assert priority `fault > lost > low battery > turn/reverse > page`, left/right segmentation, 5-second low-battery triple flash, lost-link double flash, and output ≤1000 mA after limiting.

- [ ] **Step 2: Implement hardware-independent policy**

Estimate each channel linearly against 20 mA at 255 and scale all channels with integer arithmetic when over budget. Default pixel count is 30 and budget is 1000 mA.

- [ ] **Step 3: Configure SPI3 TX DMA on PB5**

Configure SPI3 master from the existing 110 MHz PLL1Q with prescaler `/32`, giving a 3.4375 MHz effective clock, 8-bit, MSB first, TX-only DMA. Keep the LTDC PLL unchanged. Encode each GRB WS2812 bit using four SPI bits: `0` as `1000` and `1` as `1110`. For 30 pixels the encoded payload is `30 * 24 * 4 / 8 = 360` bytes, followed by 24 zero reset bytes.

The theoretical bit-cell period is 1.164 µs, with `T0H` 0.291 µs and `T1H` 0.873 µs. Because LTDC/OSPI clock constraints prevent independently selecting the earlier candidate clock without disturbing established display/memory clocks, this section was revised on 2026-07-17 after the user selected方案 A. A logic-analyzer measurement of `T0H`, `T1H`, cell period, and reset-low time on the real level-shifted DIN signal is a blocking hardware acceptance item.

```c
#define WS_ENCODED_BYTES 360u
#define WS_RESET_BYTES 24u
static uint8_t tx[WS_ENCODED_BYTES + WS_RESET_BYTES];
```

Start `HAL_SPI_Transmit_DMA()` only when idle; mark idle in the TX-complete callback. Run at 30 Hz maximum.

- [ ] **Step 4: Hardware verify electrical output**

Connect PB5 through a 3.3V→5V level shifter and 220–470Ω resistor. Verify data timing with a logic analyzer, then test 10 and 30 pixels at the 1A budget.

Expected: correct colors, no Wio reset, no visible UI frame drop, and LED current estimate never exceeds the budget.

- [ ] **Step 5: Commit**

```powershell
git add App Core/Inc/spi.h Core/Src/spi.c Core/Src/stm32h7xx_it.c Core/Src/stm32h7xx_hal_msp.c wio_ai.ioc tests Makefile cmake/st-project.cmake
git commit -m "feat: add current-limited WS2812 effects"
```

---

### Task 11: Add Diagnostics, Watchdog, and Non-Blocking Touch Probe

**Files:**
- Create: `App/Inc/platform/touch_probe.h`
- Create: `App/Src/platform/touch_probe.c`
- Create: `tests/test_diagnostics.c`
- Modify: `App/Inc/diagnostics.h`
- Modify: `App/Src/diagnostics.c`
- Modify: `App/Src/app.c`
- Modify: `App/Src/ui/ui_app.c`
- Modify: `Core/Src/main.c`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: scheduler timing, MSP/USART/LVGL/LED counters, I2C4 availability.
- Produces: `DiagnosticsSnapshot`, fault UI, watchdog gating, and optional touch capability flag.

- [ ] **Step 1: Write failing diagnostics transition tests**

Define:

```c
typedef enum { HEALTH_BOOTING, HEALTH_OK, HEALTH_DEGRADED, HEALTH_FAULT } HealthState;
typedef struct {
    HealthState state;
    uint16_t fps;
    uint32_t max_loop_us, msp_timeouts, uart_overruns, frame_misses;
    bool touch_available;
} DiagnosticsSnapshot;
```

Assert UART overrun or repeated frame misses become degraded, initialization failure becomes fault, and MSP loss remains a link condition rather than a board fault.

- [ ] **Step 2: Implement timing/counter aggregation and watchdog gate**

Only call `HAL_IWDG_Refresh()` after MSP processing, UI tick, and LED tick each set a progress bit during the current watchdog window. Configure IWDG for approximately 2 seconds in `main.c`/CubeMX-generated files.

- [ ] **Step 3: Implement bounded touch identification**

On I2C4, probe only common addresses wired to the LCD connector (`0x38` for FT5x06-family and `0x5D/0x14` for GT911-family), with one attempt per address and a 5 ms HAL timeout. Report capability only after a known chip-ID register responds. Do not implement coordinate input unless a controller is positively identified; absence is not an error.

- [ ] **Step 4: Add diagnostics overlay**

A long button press opens a panel showing FPS, MSP age/timeouts, UART overruns, max loop time, LED current estimate, reset reason, and touch availability. Severe board fault uses a red code flash; ordinary MSP loss remains yellow.

- [ ] **Step 5: Verify and commit**

Expected: host tests pass; unplugged touch does not delay boot; watchdog resets an intentionally stalled debug build within roughly 2 seconds.

```powershell
git add App Core/Src/main.c tests Makefile cmake/st-project.cmake wio_ai.ioc
git commit -m "feat: add diagnostics watchdog and touch probe"
```

---

### Task 12: Integration, Wiring Documentation, and Four-Hour Acceptance

**Files:**
- Create: `docs/hardware/wiring.md`
- Create: `docs/hardware/inav-msp-setup.md`
- Create: `docs/hardware/acceptance-results.md`
- Modify: `README.md`

**Interfaces:**
- Consumes: completed firmware and physical vehicle.
- Produces: reproducible wiring/configuration instructions and signed acceptance evidence.

- [ ] **Step 1: Document exact wiring**

`wiring.md` must contain:

```text
INAV TX  -> Wio PD9 / USART3_RX
INAV RX  <- Wio PD8 / USART3_TX
INAV GND <-> Wio GND
Wio 5V   <- independent 5V/3A buck
LED 5V   <- protected 5V LED branch
LED GND  <-> Wio GND
PB5/SPI3_MOSI -> 3.3V-to-5V level shifter -> 220–470R -> WS2812 DIN
```

Include a warning that the flight battery must never connect directly to Wio 5V.

- [ ] **Step 2: Document INAV configuration**

Specify one unused full UART set to MSP at 115200. Explicitly state not to share ELRS CRSF UART and not to select MSP DisplayPort.

- [ ] **Step 3: Run automated verification**

```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
make bsp_config_seedstudio=1 -j4
git diff --check
```

Expected: all host tests pass, firmware builds, and no whitespace errors appear.

- [ ] **Step 4: Run fault-injection matrix**

Record pass/fail and timestamps for:

1. MSP normal data and <100 ms visible response.
2. MSP unplug: stale at 500 ms, lost by 2 s.
3. MSP reconnect: normal after 500 ms stable data.
4. Bad checksum/noise injection: invalid frames ignored.
5. AUX low/mid/high and button fallback.
6. Touch absent and unknown controller.
7. 10-pixel and 30-pixel LED load.
8. Wio reset while INAV continues controlling the vehicle.
9. Buck input power cycling and motor/servo noise.

- [ ] **Step 5: Run four-hour soak**

Log start/end time, minimum FPS, maximum loop time, MSP timeout count, UART overruns, frame misses, reset count, and supply voltage. Acceptance requires no reset, unrecoverable freeze, corruption, or sustained FPS below 28.

- [ ] **Step 6: Update README and commit**

README must link the design, implementation plan, wiring, INAV setup, and acceptance results.

```powershell
git add README.md docs/hardware
git commit -m "docs: add RC crawler UI integration guide"
```

---

## Final Verification

Run:

```powershell
git status --short
git log --oneline --decorate -15
cmake --build build-host
ctest --test-dir build-host --output-on-failure
make bsp_config_seedstudio=1 -j4
```

Expected:

- Only intentionally retained or documented local changes appear.
- Every task has one focused commit.
- Native tests report `100% tests passed`.
- `build_seed/wio_ai.elf`, `.hex`, and `.bin` exist.
- Hardware acceptance evidence satisfies all criteria in the approved design.
