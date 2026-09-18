# Wio Lite AI RC Crawler UI

Wio Lite AI（STM32H725）攀爬车车壳装饰 UI 与灯光固件。它通过独立 UART 向 INAV 查询 MSP 状态，在 320×240 横屏上呈现硬派越野仪表，并根据车辆状态驱动前灯、车顶灯和四组独立 WS2812。UI 板不参与 ELRS/INAV 的遥控或动力控制链路。

## 功能一览

- **三种横屏画面**：仪表页显示油门、转向、前进/倒车、电压、姿态、GPS 卫星数和链路状态；表情页随车辆状态变化；展示页播放越野主题动态画面。页面切换带过渡动画。
- **实时 MSP 状态**：从专用 USART3（115200，8N1）轮询 RC、姿态、电压、GPS 和飞控状态。这里的“只读”指只查询状态、不向飞控下发控制命令；物理接线仍需要交叉连接 TX/RX 并共地。
- **动态尾灯**：左右各 4 颗 WS2812，正常行驶有红色追逐效果；根据 RC1 转向和 RC3 油门推断转向、刹车、倒车状态，显示对应的琥珀色流水、红色刹车和白色倒车效果。刹车是依据油门变化推断，并非读取独立刹车传感器。
- **双侧车顶灯条**：左右各 8 颗 WS2812，可选择关闭、常亮白、暖色拖尾、琥珀呼吸、彗星、彩虹追逐、车辆联动和电池/状态八种模式；车辆联动模式随油门、转向、刹车变化。AUX9 调节效果参数。
- **前灯与车顶射灯**：AUX6、AUX7 分别控制两路外置 12 V 灯的 MOSFET 开关。当前 GPIO 实现为开/关控制，尚非 PWM 无级调光。
- **独立灯组输出**：4 路、共 24 颗 WS2812（4/4/8/8），使用 **TIM2/TIM24 PWM + DMA-burst** 输出，不再使用旧版 SPI3 单链 DMA 方案。各组可以播放不同动画；3.3 V 信号需经 5 V 逻辑电平转换后接灯带。
- **演示与诊断**：无真实 MSP 数据时可显示带 `DEMO` 标识的演示画面；演示数据不会点亮实体灯。诊断页可查看 MSP age、UART 溢出、帧丢失、循环耗时和估算 LED 电流。链路丢失、低电量和故障有优先级更高的提示与保护逻辑。

## 控制方式

默认通道顺序为 AETR + AUX；灯光控制需要完整的 13 通道 `MSP_RC` 数据。飞控通道映射如与此不同，应先核对后再上车。

| 输入 | 当前功能 |
| --- | --- |
| RC1 / RC3 | 转向与油门，用于推断尾灯和车辆联动灯效 |
| AUX6 / AUX7 | 前灯 / 车顶 12 V 灯开关 |
| AUX8 | 选择车顶灯条的 8 种模式 |
| AUX9 | 调整灯效亮度或动画速度；车辆联动模式下控制整体亮度 |
| 板载 USER1 | 短按切换页面，5 秒以上切换诊断页；若 AUX 页面通道已接管，短按不覆盖其选择。2～5 秒的亮度切换目前仅预留请求，尚未接到实际背光控制 |

固件目前会探测触摸设备并在诊断页显示结果，但尚未把触摸事件接入页面切换。详细阈值、灯珠 ID 和效果优先级见[灯光验收清单](docs/hardware/lighting-checklist.md)。

## 硬件连接与安全

| 功能 | Wio 引脚 | 说明 |
| --- | --- | --- |
| MSP UART | PD8 / USART3_TX，PD9 / USART3_RX | INAV TX → PD9，INAV RX ← PD8；3.3 V、共地 |
| 前灯 / 车顶射灯 | PF3 / D10，PE10 / D11 | 驱动外置低端 MOSFET，不直接给 12 V 灯供电 |
| 左 / 右尾灯 | PA0 / D9，PB3 / D12 | 两组各 4 颗 WS2812 |
| 左 / 右车顶灯条 | PF11 / A1，PF12 / A3 | 两组各 8 颗 WS2812 |

四路 WS2812 是**各自独立的 DIN**，不是首尾相接的单条灯带；右尾灯外形镜像，但灯珠 ID 顺序与左侧相同。建议按[接线文档](docs/hardware/wiring.md)使用 74AHCT125 电平转换、独立受保护的 5 V 电源、保险丝和共地。软件的约 850 mA 是**估算限流**，不能替代电源、电流和温度的实测保护。

当前已完成软件验证，但实车灯光、线束与长期运行验收仍需按[硬件验收记录](docs/hardware/acceptance-results.md)和[灯光验收清单](docs/hardware/lighting-checklist.md)逐项确认。首次通电请断开动力系统，并对 UI 板和灯带限流供电。

## 构建与烧录

需要 GNU Make、GNU Arm Embedded Toolchain；主机测试另需 CMake、Ninja 和 CTest。在仓库根目录构建：

```powershell
make bsp_config_seedstudio=1 -j4
make -C boot_stub
```

应用产物为 `build_seed/wio_ai.{elf,bin,hex,map}`；启动桩产物为 `boot_stub/build/wio_ai_boot_stub.{elf,bin,hex,map}`。首次烧录或 sector 0 已擦除时，需要分别写入：

1. `boot_stub/wio_ai_boot_stub.hex` → `0x08000000`（sector 0 启动桩）。
2. `build_seed/wio_ai.hex` → `0x08020000`（应用）。

HEX 自带写入地址；如使用 BIN，必须手工指定上述起始地址。只有读回确认稳定启动桩仍在时，后续才可只更新应用。写 sector 0 前先确认没有需要保留的 bootloader、校准数据或其他持久化数据；不要全片擦除，也不要把应用 BIN 写到 `0x08000000`。

运行主机测试：

```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

`APP_BATTERY_CELL_COUNT` 在 `App/Inc/app_config.h` 中默认为 `0`（低电量告警禁用）；实车使用前应设为实际的 2～6 S 并重新编译。当前灯组数量固定为 4/4/8/8，旧的 `APP_LED_PIXEL_COUNT` 宏不是调整四路物理灯数的配置入口。

## 更多文档

- [INAV MSP 端口配置](docs/hardware/inav-msp-setup.md)
- [硬件接线与供电](docs/hardware/wiring.md)
- [灯光效果、通道映射与安全验收](docs/hardware/lighting-checklist.md)
- [硬件验收记录](docs/hardware/acceptance-results.md)
- [总体设计](docs/superpowers/specs/2026-07-17-rc-crawler-decoration-ui-design.md)

仓库附带 STM32CubeIDE 工程文件，可用于浏览、调试和烧录；正式构建仍以上面的 Make 命令为准。CubeMX 重新生成代码前请先审查差异，以免覆盖显示、MSP、看门狗或灯光的手写集成。
