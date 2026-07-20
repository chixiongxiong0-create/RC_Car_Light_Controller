# 硬件验收记录

本文是任务 12 的可执行验收模板。软件预检可以自动运行；涉及屏幕、飞控、灯带、供电、
触摸和看门狗复位的项目必须在真实硬件上执行。尚未执行的硬件结果统一为 `PENDING`，
不得以编译或主机单元测试标记为通过。

## 任务 12 进度

| 阶段 | 状态 | 完成条件/下一步 |
|---|---|---|
| 接线、INAV、构建/烧录文档 | READY | 实车前按现场型号补充被测配置 |
| 软件预检 | PASS（2026-07-20） | 固件硬件配置改变后需重新运行 |
| 功能与故障注入 | PENDING | 真实飞控、接收机、UI板和供电就绪后执行 |
| 屏幕阻断验收 | PENDING | 实测横屏/RGB、动态撕裂及连续60 s FPS |
| WS2812波形与电流 | PENDING | 逻辑分析仪、限流电源、10颗和30颗灯带就绪后执行 |
| 看门狗/触摸 | PENDING | 分别使用DEBUG/生产固件和实际I2C4硬件执行 |
| 四小时浸泡 | PENDING | 其余阻断项通过后执行并保存原始日志 |

当前任务 12 处于“预备完成、等待真实硬件”状态，不代表最终验收完成。

## 被测配置

| 字段 | 记录 |
|---|---|
| 固件提交 | `PENDING` |
| INAV 版本/飞控 | `PENDING` |
| MSP UART | `PENDING` |
| 电池节数（2..6 S） | `PENDING` |
| WS2812 数量 | `PENDING` |
| 降压模块/限流设置 | `PENDING` |
| 屏幕/触摸控制器 | `PENDING` |
| 测试人员 | `PENDING` |

测试前必须确认 `APP_BATTERY_CELL_COUNT` 不为默认 `0` 且等于实际电池节数；否则低电量
表达被禁用，涉及低电告警的结果无效。

## 软件预检

从仓库根目录执行：

```powershell
cmake -S tests -B build-host -G Ninja
cmake --build build-host
ctest --test-dir build-host --output-on-failure
make bsp_config_seedstudio=1 -j4
git diff --check
```

| 日期 | 主机测试 | 固件构建 | 产物 | `git diff --check` | 备注 |
|---|---|---|---|---|---|
| 2026-07-20 | PASS：CTest 4/4，100% | PASS：GNU Make exit 0；text 336300 B，data 564 B，bss 584416 B | PASS：ELF 6803592 B；BIN 336872 B；HEX 947605 B；MAP 4903975 B | PASS：exit 0 | 链接器报告ELF RWX LOAD segment警告；LF→CRLF提示；仅软件结果，不代表硬件通过 |

## 功能与故障注入矩阵

状态仅可填写 `PENDING`、`PASS`、`FAIL` 或 `BLOCKED`。证据应为带时间戳的照片、视频、
串口日志、逻辑分析仪工程或测量截图的相对路径/编号。

| ID | 前置条件 | 步骤 | 预期结果 | 状态 | 时间戳 | 证据 | 备注 |
|---|---|---|---|---|---|---|---|
| HW-01 | MSP 115200 正常、车辆安全架起 | 改变转向/油门/AUX并计时画面响应 | 数据正确，肉眼可见响应 <100 ms | PENDING | — | — | — |
| HW-02 | MSP 正常后稳定 ≥5 s | 拔掉 PD8/PD9 串口 | 约500 ms显示 stale，2 s内显示 lost；车辆控制不受影响 | PENDING | — | — | — |
| HW-03 | 已进入 lost | 恢复串口并持续发送正常数据 | 连续稳定数据约500 ms后恢复 normal | PENDING | — | — | — |
| HW-04 | 可注入串口噪声/坏校验帧 | 插入坏 checksum、截断帧和随机字节 | 无效帧被忽略，无错误状态更新、死机或阻塞 | PENDING | — | — | — |
| HW-05 | AUX 已映射、板载按键可用 | AUX依次低/中/高，再断开AUX使用按键 | 三页正确切换；按键回退可用且无抖动乱跳 | PENDING | — | — | — |
| HW-06 | I2C4可接入/断开触摸 | 分别测试无控制器、未知地址/ID、已知FT/GT | 无/未知设备不阻塞启动；FT/GT正确识别；探测总耗时 ≤15 ms | PENDING | — | — | 0x38 FT，0x5D/0x14 GT；单次5 ms |
| HW-07 | 5 V限流、10颗灯、逻辑分析仪 | 运行动画及最大允许亮度 | 颜色/顺序正确，估算和实测符合1 A预算，无UI明显掉帧/复位 | PENDING | — | — | — |
| HW-08 | 5 V/3 A、30颗灯、逻辑分析仪 | 运行动画及最大允许亮度 | 颜色/顺序正确，1 A预算生效，无过热、UI明显掉帧/复位 | PENDING | — | — | — |
| HW-09 | INAV+ELRS已可正常控车 | INAV继续运行时复位/断电Wio | 车辆控制链路不中断，飞控不重启 | PENDING | — | — | — |
| HW-10 | 记录静态供电基线 | 循环通断buck输入并施加电机/舵机负载 | Wio可恢复；INAV控制不中断；无棕断、花屏或串口锁死 | PENDING | — | — | — |
| HW-11 | 实际2..6 S已编译配置 | 模拟正常、低电阈值及恢复 | 电压显示正确，低电表达/灯效按策略触发和恢复 | PENDING | — | — | 默认0时禁止执行/签署 |

## 屏幕验收（阻断项）

当前实现是单 LTDC scanout，LVGL 写入时 LTDC 可能正在扫描，因此动态画面的撕裂验证是
发布阻断项。编译通过、静态截图正常或平均 FPS 达标均不能替代此检查。

| ID | 前置条件 | 步骤 | 预期结果 | 状态 | 时间戳 | 证据 | 备注 |
|---|---|---|---|---|---|---|---|
| DISP-01 | 屏幕已初始化 | 显示方向标记、四色块和RGB渐变 | 逻辑320×240横屏方向正确，无镜像；RGB565颜色/通道正确 | PENDING | — | — | — |
| DISP-02 | 正常数据与动态页面 | 连续观察快速遮板、条纹、数值跳变和页面切换 | 不得出现可见撕裂、错行或局部旧帧；出现即阻断验收 | PENDING | — | — | 单scanout风险 |
| DISP-03 | 动态场景持续运行 | 记录连续60 s诊断FPS | 60 s内每次采样均 ≥28 FPS，无持续掉帧 | PENDING | — | — | 附完整采样日志 |

## WS2812 波形与电流

在**电平转换器输出端、串联电阻后的 DIN 测点**测量。最终实现是 SPI3 3.4375 MHz、
4-bit `0=1000`、`1=1110`；不得引用旧的 2.4 MHz/3-bit 测量。

| ID | 条件 | T0H实测 | T1H实测 | 单元周期实测 | reset low实测 | 状态 | 时间戳/证据 | 备注 |
|---|---|---:|---:|---:|---:|---|---|---|
| WS-01 | 10颗、典型数据 | PENDING | PENDING | PENDING | PENDING | PENDING | — | 理论约0.291/0.873/1.164/55.9 µs |
| WS-02 | 30颗、典型数据 | PENDING | PENDING | PENDING | PENDING | PENDING | — | 检查最远端数据完整性 |

| ID | 条件 | 5 V静态 | 峰值电流 | 平均电流 | Wio复位 | FPS影响 | 状态 | 时间戳/证据 |
|---|---|---:|---:|---:|---|---|---|---|
| WS-03 | 10颗最大策略亮度 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | — |
| WS-04 | 30颗最大策略亮度 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | — |

## 看门狗

IWDG 标称约 2 s。`DEBUG` 构建连接调试器时会冻结 IWDG；生产构建不得依赖该冻结。

| ID | 前置条件 | 步骤 | 预期结果 | 状态 | 时间戳 | 证据 | 备注 |
|---|---|---|---|---|---|---|---|
| WD-01 | DEBUG构建、调试器连接并暂停CPU | 暂停超过5 s后继续 | 调试暂停期间IWDG冻结，不产生意外复位 | PENDING | — | — | — |
| WD-02 | 非DEBUG生产构建 | 在可控测试版本故意阻塞主循环且停止刷新 | 约2 s自动复位；记录实测间隔与复位原因 | PENDING | — | — | 恢复正式固件后再继续 |
| WD-03 | 正式固件 | 正常运行并切换所有页面/灯效/MSP状态 | 看门狗不误复位；诊断进度门控正常 | PENDING | — | — | — |

CubeMX 的 IWDG virtual pin Minor warning 是已知提示。不要为消除提示直接重新生成工程并覆盖
`Core/Src/iwdg.c` 等手写集成代码。

## 四小时浸泡测试

要求持续包含 MSP 数据变化、页面轮换、灯效和周期性电机/舵机干扰。验收要求：无复位、
不可恢复冻结、画面损坏或持续低于 28 FPS。短暂事件也必须记录，不能只填写最终值。

| 字段 | 记录 |
|---|---|
| 状态 | PENDING |
| 开始时间 | PENDING |
| 结束时间 | PENDING |
| 实际持续时间 | PENDING |
| 固件提交/构建类型 | PENDING |
| 最低 FPS | PENDING |
| 最大主循环时间（µs） | PENDING |
| MSP timeout 增量 | PENDING |
| UART overrun 增量 | PENDING |
| frame miss 增量 | PENDING |
| WS2812/DMA error 增量 | PENDING |
| reset count 增量 | PENDING |
| 5 V最小/最大电压 | PENDING |
| 电池最小/最大电压 | PENDING |
| MCU/板上最高温度 | PENDING |
| 降压模块最高温度 | PENDING |
| 灯带最高温度 | PENDING |
| 花屏/撕裂/冻结事件 | PENDING |
| 证据与日志路径 | PENDING |
| 测试人员/签名 | PENDING |
| 备注 | PENDING |

最终结论：**PENDING — 等待真实硬件完成全部阻断项和四小时浸泡测试。**
