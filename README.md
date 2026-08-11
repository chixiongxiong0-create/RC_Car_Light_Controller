# Wio Lite AI RC Crawler UI

## Crawler lighting hardware acceptance

The MSP-controlled lighting firmware has completed software verification, but vehicle acceptance remains pending. Do not connect permanent lamps or claim physical behavior until the safe bench sequence in [the lighting checklist](docs/hardware/lighting-checklist.md) is complete. It records the AUX6-AUX9 map, external MOSFET and WS2812 wiring, power protection, required real-vehicle confirmations, and the blank-board two-image flashing requirement: `boot_stub/wio_ai_boot_stub.hex` plus `build_seed/wio_ai.hex`.

Wio Lite AI（STM32H725）车壳装饰 UI 固件。它从 INAV 的独立 UART 以只读 MSP
获取车辆状态，驱动 320×240 横屏 UI，并通过 SPI3 DMA 驱动 10～30 颗 WS2812。

## 文档

- [总体设计](docs/superpowers/specs/2026-07-17-rc-crawler-decoration-ui-design.md)
- [实施计划](docs/superpowers/plans/2026-07-17-rc-crawler-decoration-ui-implementation.md)
- [接线与供电](docs/hardware/wiring.md)
- [INAV MSP 配置](docs/hardware/inav-msp-setup.md)
- [硬件验收记录](docs/hardware/acceptance-results.md)

硬件验收文档中的 `PENDING` 项必须在真实车辆上完成，不可由主机测试或编译结果代替。

## 配置

烧录前检查 `App/Inc/app_config.h`：

- `APP_BATTERY_CELL_COUNT` 默认为 `0`（未知），此时低电量表达和告警被禁用。
  实车必须设置为实际的 `2..6` S。
- `APP_LED_PIXEL_COUNT` 设置实际灯珠总数，范围为 `4..30`；前四颗为后部灯组。

## 构建

需要 GNU Make、GNU Arm Embedded Toolchain、CMake、Ninja 和 CTest。所有命令均从
仓库根目录运行，不依赖某台电脑上的绝对路径。

```powershell
make bsp_config_seedstudio=1 -j4
```

产物位于：

```text
build_seed/wio_ai.elf
build_seed/wio_ai.bin
build_seed/wio_ai.hex
build_seed/wio_ai.map
```

应用固定使用 `STM32H725AEIX_PSRAM.ld`，向量表位于 `0x08020000`。sector 0 的稳定
启动桩独立构建；它的固定入口会在每次启动时从 `0x08020000` 动态读取应用 MSP 和
`Reset_Handler`，因此应用重链接后无需更新桩内地址：

```powershell
make -C boot_stub
```

生成 `boot_stub/build/wio_ai_boot_stub.{elf,hex,bin,map}`。仓库还跟踪了经相同源码生成的
`boot_stub/wio_ai_boot_stub.hex`，供烧录和 fresh build 比对。根 ARM CMake 构建也会同时
生成应用和该启动桩，并在所有构建配置中使用相同的 `0x08020000` 应用布局：

```powershell
cmake -S . -B build-arm -G Ninja `
  '-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake' `
  '-DCMAKE_BUILD_TYPE=Release'
cmake --build build-arm
```

完成启动桩与应用 Make 构建后，可执行工件检查，验证两侧向量地址、动态跳转反汇编、当前应用
入口未嵌入启动桩，以及 fresh/tracked HEX 记录一致：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File scripts/inspect_boot_stub.ps1 `
  -BootStubElf boot_stub/build/wio_ai_boot_stub.elf `
  -BootStubBin boot_stub/build/wio_ai_boot_stub.bin `
  -ApplicationElf build_seed/wio_ai.elf `
  -BootStubHex boot_stub/build/wio_ai_boot_stub.hex `
  -TrackedHex boot_stub/wio_ai_boot_stub.hex
```

清理并重建：

```powershell
make bsp_config_seedstudio=1 clean
make bsp_config_seedstudio=1 -j4
```

运行主机测试：

```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

## 烧录

可使用 STM32CubeProgrammer、STM32CubeIDE 或 J-Link，通过板载/外接调试器下载。优先使用
Intel HEX，因为文件自带目标地址。完整可启动布局由两个工件组成：

1. 将 `boot_stub/wio_ai_boot_stub.hex` 写入 sector 0；其向量表从 `0x08000000` 开始。
2. 将 `build_seed/wio_ai.hex` 写入应用区；其向量表从 `0x08020000` 开始。

**空白设备、sector 0 已擦除的设备，或仍装有旧式绝对跳转向量的设备，必须写入两者；只写
应用 HEX 不会让空白设备启动。** 只有已读回确认稳定启动桩存在时，后续应用升级才可只写
地址化应用 HEX。启动桩在运行时读取应用向量，因此未来 `Reset_Handler` 移动不需要重建或
修改 sector 0 跳转地址。

若工具只能写原始 BIN，`boot_stub/build/wio_ai_boot_stub.bin` 的起始地址必须是
`0x08000000`，`build_seed/wio_ai.bin` 的起始地址必须是 `0x08020000`，不得互换。写启动桩
通常需要擦除整个 128 KiB sector 0；执行前必须确认其中没有其他 bootloader、校准或持久化
数据。不要执行全片擦除，也不要把应用 BIN 写到 `0x08000000`。首次上车前先断开动力系统，
仅给 UI 板和灯带限流供电。

### 2026-08-04 稳定启动桩硬件证据

J-Link V8.18 已将稳定启动桩和 fresh 地址化应用分别写入并完成 `Verify O.K.`。sector 0
读回向量为 `SP=0x24050000`、`reset=0x08000041`，固定代码从 `0x08000040` 动态读取应用
向量；`0x08020000` 的应用向量读回为 `0x24050000 / 0x0805CD85`（后者包含 Thumb bit）。

复位 3 秒后的探针值为 `PC=0x08023E88`、`IPSR=0`、`VTOR=0x08020000`；稍后的运行时
探针值为 `PC=0x0805EE78`、`IPSR=0`、GPIOF `ODR=0x20`（PF5 高），且
`CFSR=0`、`HFSR=0`。两次脚本最后均执行 `go`。

这些证据仅确认稳定启动桩烧录/读回、复位进入 fresh 应用、短时运行和 PF5 高为 `PASS`。
它们不构成连续或长时间运行证明；三页视觉、`USER1`、`DEMO` 徽标、实时 MSP 接管、
60 秒动态观察和四小时浸泡仍为 `PENDING`。

## GUI 工具

仓库包含 `.project`、`.cproject`、`.mxproject` 和 `wio_ai.ioc`，可导入
STM32CubeIDE 用于浏览、编辑、烧录和调试。正式构建仍以以上 Make 命令为准，因为
CubeIDE managed build 不保证包含全部 `App/` 与 LVGL 源文件。

CubeMX 中 IWDG 的 virtual pin 可能显示 Minor warning；IWDG 的实际初始化由
`Core/Src/iwdg.c` 手工维护。不要未经差异审查直接从 CubeMX 重新生成代码，否则可能
覆盖手写的看门狗、显示、MSP 和 WS2812 集成。
