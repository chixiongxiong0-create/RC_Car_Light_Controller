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

## 地址化 HEX 烧录：历史发现与当前状态

该构建的 `STM32H725AEIX_PSRAM.ld` 将 `.isr_vector` 放在 `0x08020000`。后续烧录优先
使用携带地址的 `wio_ai.hex`；若只能使用原始 BIN，必须显式写入 `0x08020000`。在目标
boot layout 已被有意确定前，禁止全片擦除。

**历史发现（不构成本次验收）**：无地址 BIN 曾被误写到 `0x08000000`，导致应用无法进入；
随后一次地址化 HEX 操作曾观察到正常执行和 PF5 高电平。这项历史观察不等同于当前板卡的
通过结论，必须在完整启动布局恢复后重新验证。

2026-08-03 的全新构建通过主机 CTest 6/6、固件 Make 构建，且 ELF/HEX 分别确认
`.isr_vector = 0x08020000` 与扩展地址记录 `:020000040802F0`。2026-08-18 的恢复操作已
成功写入并校验 `0x08020000` 的应用区域；新镜像向量为 `SP=0x24050000`、
`Reset_Handler=0x0805CCD9`，应用所需 VTOR 偏移为 `0x00020000`。但复位仍需要
`0x08000000` 的最小 sector 0 启动向量；由于尚未获得对这个具体高风险 boot layout 操作的
明确批准，该向量没有写入，复位结果为 HardFault。

因此当前硬件执行和 PF5 电平都是 `PENDING`，不是 PASS。以下可视验收同样为 `PENDING`，
必须由测试人员在真实硬件上观察并留存证据：三个页面的动画数值、`USER1` 页面切换、可见
`DEMO` 徽标，以及实时 MSP 接管；这些项目不因 J-Link 或主机验证而通过。

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

### 采集方法与来源

- 诊断 overlay 是固件计数器的签署来源，可见字段为：`fps`、当前
  `frame_misses`、`max_loop`、`msp_timeouts`、`uart_overruns`、`reset_flags` 和
  `touch`。开始和结束各拍一张清晰照片；测试中每 10 分钟拍照或从连续录屏提取一帧。
- `msp_timeouts` 和 `uart_overruns` 可用结束值减开始值得到差值。`frame_misses` 按每次
  采样的可见值记录，并签署“最大观察到的连续 frame_misses”，不声称它是累计 delta。
- FPS 以定时采样值签署“最低采样 FPS”；画面连续性和复位由覆盖整个测试的连续视频
  佐证，不能由单张结束照片推断。
- 当前诊断 overlay 没有持久的 WS2812/DMA error 计数。改为从连续视频和人工事件日志
  记录“可见灯效停更/恢复事件数”；事件发生时再保存逻辑分析仪触发记录。若必须获得
  内部 WS 错误精确值，填写 `N/A—需另加 instrumented build`，它不是当前固件的签署字段。
- 当前固件也没有持久 reset count。复位次数根据连续视频中的 boot sequence 和电源电流
  重启特征人工计数；事件后立即拍摄 `reset_flags`。该值不是内部持久计数器。
- 5 V/电池电压由带日志的万用表或示波器采集；温度由热电偶或红外测温仪采集，并保存
  仪器型号、测点和原始记录。

### 开始/结束诊断快照

| 快照 | 时间戳 | fps | frame_misses（当前） | max_loop (µs) | msp_timeouts | uart_overruns | reset_flags | touch | 照片/录屏证据 |
|---|---|---:|---:|---:|---:|---:|---|---|---|
| START | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| END | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |

### 每10分钟定时采样

从 `T+000` 开始，每 10 分钟增加一行，直到 `T+240`（共25个采样点）。每行必须关联
诊断 overlay 的照片或连续录屏时间码；不得从缺失采样点内插通过结果。

| T+分钟 | 时间戳 | fps | frame_misses（当前） | max_loop (µs) | msp_timeouts | uart_overruns | 5 V | 电池V | 板/降压/灯带温度 | 视频时间码/照片 |
|---:|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| 000 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 010 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 020 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 030 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 040 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 050 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 060 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 070 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 080 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 090 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 100 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 110 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 120 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 130 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 140 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 150 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 160 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 170 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 180 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 190 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 200 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 210 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 220 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 230 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |
| 240 | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING | PENDING |

### 人工事件日志

| 时间戳/视频时间码 | 事件类型 | 现象与恢复 | boot sequence/电流重启特征 | 事件后reset_flags | 逻辑分析仪/其他证据 |
|---|---|---|---|---|---|
| PENDING | 灯效停更/恢复、复位、撕裂、冻结或其他 | PENDING | PENDING | PENDING | PENDING |

### 四小时签署汇总

| 字段 | 来源 | 记录 |
|---|---|---|
| 状态 | 全部阻断项与下列证据 | PENDING |
| 开始/结束时间、实际时长 | START/END照片元数据及连续视频 | PENDING |
| 固件提交/构建类型 | ELF对应提交与构建命令记录 | PENDING |
| 最低采样 FPS | 每10分钟采样表中的最小值 | PENDING |
| 最大主循环时间（µs） | overlay `max_loop`各采样最大值 | PENDING |
| 最大观察到的连续 frame_misses | overlay当前值及连续视频 | PENDING |
| MSP timeout 开始/结束/差值 | START/END `msp_timeouts` | PENDING |
| UART overrun 开始/结束/差值 | START/END `uart_overruns` | PENDING |
| 可见灯效停更/恢复事件数 | 连续视频＋人工事件日志；发生时附逻辑分析仪触发 | PENDING |
| 观察到的复位次数 | 连续视频boot sequence＋电源电流特征；非持久内部counter | PENDING |
| 复位事件后的 reset_flags | 事件后overlay照片 | PENDING |
| 5 V最小/最大电压 | 万用表/示波器原始日志 | PENDING |
| 电池最小/最大电压 | 万用表/示波器原始日志 | PENDING |
| 板上/降压模块/灯带最高温度 | 已记录测点的测温仪原始数据 | PENDING |
| 花屏/撕裂/不可恢复冻结事件 | 连续视频＋人工事件日志 | PENDING |
| 证据与日志路径 | 照片、视频、测量与逻辑分析仪文件清单 | PENDING |
| 测试人员/签名 | 人工签署 | PENDING |
| 备注 | 人工记录 | PENDING |

最终结论：**PENDING — 等待真实硬件完成全部阻断项和四小时浸泡测试。**
