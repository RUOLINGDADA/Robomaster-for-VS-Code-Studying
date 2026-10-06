# DBUS 遥控输入

`Core/Inc|Src/bsp/dbus` 使用 USART3 PC11、100000 baud、9B+偶校验、DMA1 Stream1
双缓冲接收 18 字节 DJI DBUS 帧。9B 包含 8 位数据和 1 位校验；配置成 8B+偶校验会
把数据最高位当成校验位，摇杆高位出现时会产生错误控制量。

DMA 中断只复制完成块到固定 16 槽静态环形邮箱，`task_dbus` 每 2 ms 解码并发布一次
Yaw CH0、Pitch CH1 命令。覆盖邮箱只保留最新状态，避免积压旧摇杆动作；命令时间戳
使用接收时刻，100 ms 没有合法帧即掉线。DBUS 掉线或快照失效使命令失效：云台停止
目标移动并保持位置，供弹停止；云台 CAN 反馈掉线才清零。鼠标 X/Y 由虚拟摇杆模块
按合法帧序列只消费一次，用浮点增益累计为虚拟速度；停止后按各轴任务配置保持并线性回中。
DBUS 合法帧先进入固定环形邮箱，再由任务一次排空并累计本周期的
`mouse_delta_x/y`。快照时间使用最后一帧接收时刻，避免积压帧因旧时间戳立即进入回中。
邮箱满时递增 `frame_overruns`；日志能区分累计输入和真实丢帧。
摇杆与虚拟鼠标速度合成后限制为 ±1000‰；鼠标左键发布连续发射命令。

中断优先级为 5，满足 FreeRTOS `FromISR` 规则；USART3 不调用 `HAL_UART_IRQHandler`，
因为 HAL 单缓冲接收会 Abort 自定义 DBM。半帧 IDLE、UART/DMA 错误只请求任务恢复，
不会在 ISR 中打印或阻塞。驱动没有 CRC，字段范围校验只能过滤明显错位，不能替代实机
链路检查。

任务配置位于 `Core/Inc/task/task_dbus/task_dbus_config.h`：摇杆反向调
`DBUS_*_INPUT_SIGN`，回中漂移调 `DBUS_STICK_DEADBAND_RAW`，满杆速度调对应轴
`*_MAX_COMMAND_SPEED_RPM`；输入方向和 GM6020 电流方向是两个坐标系，不能混改。
虚拟鼠标总开关和公共输出上限也在该配置头中；Yaw/Pitch 的增益、方向、保持和回中分别配置。
增益可设 `0.1f`、`0.05f` 或 `0.01f`；累计、回中和速度命令保留小数，图表按整数协议输出。关闭虚拟鼠标后只由摇杆产生速度，
不回退到旧的逐周期鼠标映射。算法接口、状态定义和调参说明见
`doc/modules/mouse_virtual_joystick/README.md`。

## 串口遥测与图表帧

USART1 是日志输出口，USART3 只接收 DBUS。`Core/Inc/app/log/log_config.h` 提供编译期
开关：`LOG_GLOBAL_ENABLE` 和 `LOG_USART1_ENABLE` 是总门，`LOG_DBUS_TEXT_ENABLE` 控制
完整文本，`LOG_DBUS_CHART_ENABLE` 控制图表帧，`LOG_TASK_ENABLE` 和 `LOG_TEST_ENABLE`
分别控制正式任务与硬件测试诊断，并与 `LOG_YAW_ENABLE/LOG_PITCH_ENABLE/LOG_FEED_MOTOR_ENABLE`
配合。所有输出直接使用 `app/log/log.h` 的 `LOG_TRY_PRINTF`；关闭分类后对应格式参数不求值、不占用 DMA。

默认每 `DBUS_TASK_LOG_PERIOD_MS=100 ms` 尝试一次；周期由 DBUS 任务配置独立管理。文本和图表启用时合并为一次 USART1 DMA
提交；USART1 忙或文本超过 768 字节时整条丢弃，下一个周期重试。DBUS 任务仍使用同一份
快照发布命令，日志不会刷新命令时间戳。

图表格式兼容 `hehe.html` 的文本 CSV 解析：

```text
ch:CH0,CH1,CH2,CH3,CH4,virtual_mouse_x,virtual_mouse_y,mouse_z,status,keyboard\n
```

通道 0~4 是已减中心值的 DBUS 摇杆，通道 5/6 是虚拟鼠标 X/Y，通道 7 是原始鼠标 Z，
通道 8 是状态位图，通道 9 是键盘位图。原始鼠标 X/Y 保留在完整文本日志中。状态位图
bit0~1 为左拨杆，bit2~3 为右拨杆，bit4/5 为鼠标左/右键。
文本日志还包含拨杆、鼠标、键盘、在线状态、帧年龄、错误计数和 Yaw/Pitch/fire 派生命令。
新增 `virtual_mouse_x_x100/y_x100`（‰×100）、`mouse_virtual_state`、`mouse_virtual_x_state/y_state`、`mouse_frame_sequence`、
`mouse_delta_x/y`、`mouse_delta_frames` 和 `frame_overruns`；原始鼠标字段保持 count 单位。
合法帧计数同时作为去重序号，打印不改变输入有效期。
离线时保留最后合法数据用于诊断，但 `online=0`，命令邮箱同时失效。
完整配置、键盘位映射、DMA 带宽及多多盒子设置见 `doc/modules/log/README.md`。

验证：临时主机测试覆盖 11 位通道高位、鼠标有符号值、拨杆编码、M0/M1 所有权、IDLE
顺序、错误恢复、时间回绕、覆盖邮箱和双轴命令超时；`cmake --preset Debug` 与构建通过。
本轮临时替身额外覆盖实际任务的虚拟鼠标、零位移回中、离线清零、DMA 忙、10 通道解析和
32 组日志/虚拟开关运行；原始 USART3 DMA/IRQ 未改变。实际鼠标手感和摆动仍需上板验证。
