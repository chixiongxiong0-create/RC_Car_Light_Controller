# INAV MSP 配置

## 端口选择

在 INAV Configurator 的 Ports 页面选择一个**未使用的完整硬件 UART**：

- 不得选择已经承载 ELRS/CRSF 接收机的 UART。
- 不得与 GPS、VTX、串口接收机或其他外设共享。
- 将该 UART 的 MSP 功能设为 `115200` baud。
- 不要启用 `MSP DisplayPort`；本固件使用普通 MSP 请求/响应，不是 OSD DisplayPort。

保存并重启飞控后，再按[接线说明](wiring.md)交叉连接 TX/RX 并共地：

```text
INAV TX -> Wio PD9 / USART3_RX
INAV RX <- Wio PD8 / USART3_TX
```

UART 参数为 115200、8 数据位、无校验、1 停止位、非反相、3.3 V 电平。

## 配置和验证顺序

1. 记录 ELRS/CRSF 所在 UART，确认不会改动它。
2. 给空闲 UART 启用普通 MSP 115200，保存并重启。
3. 螺旋桨/传动机构保持安全状态，只给电子系统供电。
4. 确认 Wio UI 收到数据，诊断页的 MSP age 持续更新。
5. 依次验证 AUX 低/中/高页面切换；再验证板载按键回退。
6. 拔掉 MSP 串口，确认约 500 ms 进入 stale、2 s 内进入 lost。
7. 重连并提供连续稳定数据，确认约 500 ms 后恢复正常。

Wio UI 板只读取状态和发送 MSP 查询，不参与飞控的控制链路。复位或断开 Wio 时，INAV
和 ELRS 必须继续正常控制车辆；此项需要在实车验收中确认。

## 电池节数

固件的 `APP_BATTERY_CELL_COUNT` 默认为 `0`，表示未知并禁用低电量表达。真实硬件测试
前必须在 `App/Inc/app_config.h` 中改为实车的 `2..6` S，再重新编译和烧录。不要仅依赖
INAV 自动检测替代此编译期配置。
