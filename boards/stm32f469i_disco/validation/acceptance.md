# MB1189 B-01 上板验收记录

记录日期：2026-09-24。状态 `PASS` 仅代表已执行并有结果；`PENDING` 代表尚未实测。

| 项目 | 状态 | 记录 / 通过条件 |
| --- | --- | --- |
| 主机逻辑测试 | PASS | 15/15 CTest 通过，含双链 8/16 灯珠顺序。 |
| F469 固件构建 | PASS | GNU Arm 工具链生成 ELF/HEX/BIN；FLASH 约 605 KB，内部 RAM 静态占用约 172 KB。 |
| ST-LINK 单板连接与固件运行 | PASS | 识别 STM32F469、2 MB Flash、3.04 V；烧录后校验成功；运行计时递增，SystemCoreClock 为 180 MHz，诊断健康 OK，界面刷新约 29–30 FPS。原始 2 MB Flash 已备份在 `E:/workspace/fpv/hardware_backups/mb1189-b01-before-f469-port-2026-09-24.bin`。 |
| 实板 SDRAM 画面读回 | PASS | 经 ST-LINK 读回 800×480 帧缓冲，已检查表情、总览、详细诊断三张图；见本目录 PNG。画面采集期间的局部撕裂不代表屏幕实物问题。 |
| WS2812 定时器 DMA 软件状态 | PASS | 板上读数：TIM3 ARR=112，90 MHz 计时对应约 796.5 kHz；CCR 目标值 32/63 tick（约 356/700 ns）；DMA 完成计数持续增加，错误计数 0。DIN 波形仍待示波器确认。 |
| B-01 晶振与候选引脚 | PENDING | 核对 8/25 MHz 晶振与 CN7/CN12 信号实际连通，排除板上外设占用。 |
| 单板 800×480 显示观感 | PENDING | 请观察两屏有无剪裁、闪烁、花屏；记录供电电流。 |
| 触摸与 USER 回退 | PENDING | 点按两个主标签；两页内左右滑动；测试触摸不可用时 USER 按钮。 |
| MSP 独立 UART | PENDING | 3.3 V、交叉 TX/RX、共地；确认总览数据与诊断 MSP age 更新。 |
| MSP 断连 / 低电压 | PENDING | 断连后值转未知、表情告警；低电压提示优先且实体灯安全。 |
| WS2812 A 8 灯 | PENDING | 示波器测 DIN，经 74AHCT125 与 330 Ω；限流 5 V 测时序、顺序、电流。 |
| WS2812 B 16 灯 | PENDING | 同上；确认 8+8 顺序和长链尾部。 |
| 两路 12 V 灯 | PENDING | 先核对 MOSFET、栅极下拉与保险丝，再单独限流上电，验 AUX6 三段。 |
| 掉电 / 复位 / 看门狗 | PENDING | 反复复位和电源波动时灯默认低，界面可恢复。 |
| 整车供电与 INAV 独立性 | PENDING | 测总电流、地线压降；拔掉 UI 板后 INAV / ELRS 仍可控制车辆。 |

首次上板顺序：仅 Discovery → 屏幕与触摸 → 3.3 V MSP → 两条限流 5 V LED → 检查 MOSFET 后接入 12 V 灯。每一步记录电压、电流和异常；上一步未通过则不继续接线。
