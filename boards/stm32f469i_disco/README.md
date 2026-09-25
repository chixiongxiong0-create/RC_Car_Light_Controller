# STM32F469I-DISCO 界面与灯光固件

目标板丝印：**MB1189 B-01**。这一路固件替代原 Wio Lite AI 的显示、触摸、界面、MSP 状态读取、两路 12 V 灯控制和两条 WS2812 数据链。ELRS、INAV 与动力控制仍由原飞控系统承担。

## 功能

- 800×480 触摸屏有「表情」和「信息」两个主界面。顶部触摸标签切换主界面，左右滑动切换当前界面的内容。表情随车况自动变化，手动滑动选择保持 10 秒；低电压和链路丢失优先。信息有总览和详细诊断两页。
- 信息总览页底部有背光亮度滑条，范围 10%–100%，拖动时通过 DSI 指令即时调节；每次开机默认 100%。
- MSP 使用 CN12 6/8 脚上的 USART6，115200 8N1，只查询状态。无有效 MSP 时界面可显示 `DEMO`，实体灯始终读取真实车况，不使用演示数据。
- 两条 WS2812 输出使用 CN12 5/9 脚，分别串接逻辑组 4+4 与 8+8，共 8/16 灯。AUX6 控制两路灯开关，AUX8/AUX9 控制原有 LED 模式和效果。
- 触摸初始化失败时，板载 USER 按钮切换两个主界面。初始化失败会令灯输出保持低电平。

显示驱动按 Discovery BSP 的 ARGB8888 层格式使用外部 SDRAM 帧缓冲（800×480×4 字节），LVGL 使用内部 RAM 的 40 行 RGB565 绘图缓冲并在刷新时转换。帧率与响应速度须在实板验收。

## 构建

需要 CMake、Ninja、GNU Arm 工具链与 STM32CubeF4 HAL/BSP。仓库包含 LVGL 子模块。以下命令在仓库根目录执行：

```powershell
git submodule update --init Middlewares/Third_Party/lvgl
cmake -S . -B build-f469 -G Ninja -DBUILD_F469_UI=ON -DF469_DISCO_REVA=OFF -DSTM32CUBE_F4_ROOT=D:/development/st/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3 -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
cmake --build build-f469 --target f469_ui -j 8
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

产物位于 `build-f469/boards/stm32f469i_disco/f469_ui.{elf,hex,bin}`。`F469_DISCO_REVA=OFF` 采用 8 MHz HSE 与 Discovery Rev B 配置；`ON` 采用 25 MHz HSE 与 Rev A 配置。B-01 丝印表明早期板号，但此板的实际振荡器尚未测量，因此刷写前应核对晶振标识与 ST-LINK 连接状态。仅在板子不接车载外设时做首次上电与显示验证。

## 接线与验收

候选引脚见 [pin-map.md](pin-map.md)。公开的 MB1189 原理图未覆盖 B-01，候选接线仍需按实物和对应版次核实。WS2812 数据须经过 74AHCT125 等 5 V 电平转换，灯与 LED 分别使用受保护的独立 12 V / 5 V 电源；板上 GPIO 只驱动 MOSFET 门极，不能直接连接 12 V。按 [acceptance.md](validation/acceptance.md) 的顺序验证后再装车。

固件已在连接的 MB1189 B-01 上烧录并校验，运行约 29–30 FPS；ST-LINK 读回了表情、总览和详细诊断三张 800×480 画面。屏幕实际观感、触摸手势、外接 MSP、WS2812 DIN 波形、12 V 灯开关与整车供电仍需实物验收。
