# 接线与供电

## 信号接线

| 来源 | 方向 | Wio Lite AI | 说明 |
|---|---:|---|---|
| INAV UART TX | → | PD9 / USART3_RX | 3.3 V UART |
| INAV UART RX | ← | PD8 / USART3_TX | 3.3 V UART |
| INAV GND | ↔ | Wio GND | 必须共地 |

USART3 必须独占给 MSP，不能与 ELRS/CRSF UART 共用。TX/RX 交叉连接；即便当前固件
主要读取数据，也应保留完整双向 UART，以便正常发送 MSP 请求。

## 四路 WS2812 数据接线

四条数据网彼此独立；**不要**将 WS1--WS4 串接成一条数据链。连接器顺序、像素数和
串联电阻必须如下：

```text
PA0/D9   -> 100R -> 74AHCT125 1A/1Y -> 330R -> WS1 DIN (4 pixels)
PB3/D12  -> 100R -> 74AHCT125 2A/2Y -> 330R -> WS2 DIN (4 pixels)
PB4/MISO -> 100R -> 74AHCT125 3A/3Y -> 330R -> WS3 DIN (8 pixels)
PB5/MOSI -> 100R -> 74AHCT125 4A/4Y -> 330R -> WS4 DIN (8 pixels)
```

WS1、WS2、WS3、WS4 的顺序是固件输出组 0、1、2、3，固定为 **4 / 4 / 8 / 8**，
总计 24 颗像素。连接每一组之前先确认其 DIN 方向；不应由 WS1 的 DOUT 驱动 WS2，
也不应混接四个输出。

使用一个由受保护 5 V 供电的 **74AHCT125**。其四个低有效 OE 引脚均接地；在芯片 VCC
与 GND 间、靠近芯片放置 **100 nF** 本地去耦。每个 MCU 输出先经过 100 Ω，再进入对应
AHCT 输入；每个 5 V AHCT 输出再经 330 Ω 接到对应的 WS DIN。AHCT 输入门限可可靠识别
STM32 的 3.3 V 高电平，输出只朝像素端驱动。

**绝不允许 5 V 逻辑反馈到 STM32 引脚。** 禁止将 AHCT 输出、WS2812 DOUT 或其他 5 V
源直接接至 PA0、PB3、PB4 或 PB5；禁止使用 BSS138、双向 MOSFET 或 I2C 用自动双向
电平转换模块。

## 供电与保护

```text
动力电池 -> 独立、保险保护的 5 V / 3 A BEC -> 受保护的 LED 5 V 分支
                                                  +-> 74AHCT125 VCC
                                                  +-> WS1..WS4 5 V

Wio GND <-> INAV GND <-> BEC GND <-> 74AHCT125 GND <-> WS1..WS4 GND
```

> **危险：动力电池绝不能直接接入 Wio 的 5 V 引脚。** 使用额定至少 5 V / 3 A 的 BEC，
> LED 5 V 分支必须有保险丝或自恢复保险丝，并在受保护输入端放置足够的 bulk 电容。

所有设备必须共地；大电流回路不得经过 Wio 板或细信号地线。每个远端 WS 组可选加
100 µF 电容。固件的 **1 A 软件预算**覆盖全部 24 颗像素，但不能替代 BEC、保险丝、
线径、bulk 电容、TVS 或限流电源的硬件保护。首次接通只能使用限流电源，先逐组验证，
再验证全部负载。

## 12 V 灯具接线

两个外置低边 **BSZ028N04LS** MOSFET 开关均为高有效：PF3 / Arduino D10 控制前灯，
PE10 / Arduino D11 控制顶灯。每个 gate 采用 GPIO -> 47--100 Ω 串联电阻 -> gate，
并加 10 kΩ gate-to-ground 下拉。

每个灯具回路：`battery positive -> fuse -> lamp positive`；`lamp negative -> MOSFET drain`；
`MOSFET source -> battery negative`。在灯具供电侧安装计划的 bulk 电容和 TVS，并与 Wio、
LED 电源、INAV 和电池共地。不得让灯具电流经过 Wio。

## WS2812 时序

PWM/DMA 传输对每个输出组独立发送 GRB/MSB 数据，并在每帧后附带复位低电平。实际
波形会受定时器时钟、电平转换器、线束和器件批次影响，必须在 AHCT 输出端、330 Ω 后
的 DIN 测点用示波器/逻辑分析仪测量。所有波形、复位、顺序、温升和电流测量目前均为
`PENDING`，不能由构建或主机测试替代。

## 上电前检查

1. BEC 空载输出为 5 V，极性正确，LED 支路保险丝、受保护输入 bulk 电容和可选远端
   100 µF 电容已安装。
2. INAV、Wio、74AHCT125 与 WS1--WS4 共地，动力回路大电流不经过信号地细线。
3. USART3 未连接 ELRS 接收机，PD8/PD9 无其他外设占用。
4. PA0/PB3/PB4/PB5 各自按上图通过 100 Ω、5 V 74AHCT125 通道和 330 Ω 接到对应 DIN；
   四个 OE 均为低，100 nF 去耦正确，且无 5 V 数据反灌到 MCU。
5. 已核对 WS1=4、WS2=4、WS3=8、WS4=8，未把任何组串接。
6. `APP_BATTERY_CELL_COUNT` 已设置为实车 `2..6` S；默认 `0` 不产生低电告警。

## 烧录限制

本次四路 WS2812 文档和软件验证**未烧录、未连接或操作任何硬件**。空白板或没有已确认
稳定 sector-0 stub 的板，日后应先烧录 `boot_stub/wio_ai_boot_stub.hex` 至 `0x08000000`，
再烧录应用 `build_ws4_final/wio_ai.hex` 至 `0x08020000`；本任务不执行该操作。
