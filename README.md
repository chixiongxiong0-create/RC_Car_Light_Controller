# Wio Lite AI RC Crawler UI

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
- `APP_LED_PIXEL_COUNT` 设置实际灯珠数，范围为 `10..30`。

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

可使用 STM32CubeProgrammer 或 STM32CubeIDE，通过板载/外接 ST-LINK 下载
`build_seed/wio_ai.elf` 或 `.hex`。若烧录 `.bin`，起始地址为 STM32H725 内部
Flash 地址 `0x08000000`。首次上车前先断开动力系统，仅给 UI 板和灯带限流供电。

## GUI 工具

仓库包含 `.project`、`.cproject`、`.mxproject` 和 `wio_ai.ioc`，可导入
STM32CubeIDE 用于浏览、编辑、烧录和调试。正式构建仍以以上 Make 命令为准，因为
CubeIDE managed build 不保证包含全部 `App/` 与 LVGL 源文件。

CubeMX 中 IWDG 的 virtual pin 可能显示 Minor warning；IWDG 的实际初始化由
`Core/Src/iwdg.c` 手工维护。不要未经差异审查直接从 CubeMX 重新生成代码，否则可能
覆盖手写的看门狗、显示、MSP 和 WS2812 集成。
