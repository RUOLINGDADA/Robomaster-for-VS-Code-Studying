# C615 与 Snail 2305 PWM 驱动

## 作用

`bsp/snail_2305` 把 TIM1 CH1/CH2 的比较值转换为 C615 PWM 脉宽。它只处理
TIM HAL、脉宽范围、停止值和非阻塞斜坡；不判断电机转速，不解析 DBUS，也不实现
发射状态机。

本工程使用 PE9/TIM1_CH1 和 PE11/TIM1_CH2。TIM1 计数频率为 1 MHz，周期为
19999，因此 PWM 周期为 20 ms（50 Hz）。C615 手册允许 400~2200 us、最高
500 Hz；本工程的校准运行范围为 1000~2000 us。

## 接口

- `Snail2305_Init()`：检查 TIM1 实例、通道、PSC、ARR 和范围，先写停止脉宽再启动 PWM。
- `Snail2305_SetPulse()`：立即写脉宽，用于安全停机和初始化。
- `Snail2305_SetStopPulse()`：更新停止脉宽并立即写入 CCR，适合校准后调参。
- `Snail2305_SetTargetPulse()`：只修改斜坡目标。
- `Snail2305_SetOutput()`：把 0~1000‰ 映射到停止到最大脉宽。
- `Snail2305_Process()`：按任务周期推进斜坡并更新 CCR。
- `Snail2305_Stop()`：立即写停止脉宽。

C615 没有本项目使用的 CAN 反馈。`initialized=true` 只说明 HAL 接受了 PWM
启动，不说明电调已上电、电机已转动或转速达到目标。

`SNAIL_2305_TEST_CH1_DIRECTION_SIGN`、`SNAIL_2305_TEST_CH2_DIRECTION_SIGN`
和正式配置中的 `FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN`、
`FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN` 取 `+1/-1`，用于分别记录和校验两路期望
转向。C615 的 PWM 脉宽本身只有停止到油门范围，不能像带符号电流的 CAN 电机一样
在软件中直接反向；确认方向后仍需交换任意两根相线，或在 DJI Assistant 2 开启快速
转向并设置方向。测试和正式模式的宏可以分别覆盖，但应保持实际硬件设置一致。

## 任务组合

现有 `task_feed_motor` 每 2 ms 同时管理两个 C615 和 CAN1 ID1 的 M2006。DBUS
在线且左键按下时，两路目标为 1520 us；从左键命令生效起经过 300 ms 且两个 PWM 已到达活动值后，M2006
开始执行 409 count 的角度步进。误差进入 65 count、逻辑速度低于 10 rpm 并
连续保持 20 ms 后，等待 100 ms 再推进下一步。

DBUS 离线、M2006 反馈超时、初始化失败或左键释放时，M2006 发送零电流；C615
故障时直接写 1000 us，正常释放时按斜坡回到 1000 us。两个轮使用同一脉宽，
相反旋转方向由 Snail 相线或 DJI Assistant 设置完成。

## CubeMX 对接

`.ioc` 持久化 TIM1、PE9、PE11、PSC=167、ARR=19999 和两个初始 Pulse=1000。
CubeMX 重新生成后必须确认 `tim.c/h`、TIM1 MSP 和 `cmake/stm32cubemx/CMakeLists.txt`
仍存在。PWM 启动属于任务上下文，不需要 TIM1 中断或 FreeRTOS ISR API。

第一次上电前按 C615 手册完成 PWM 行程校准，并准备急停。正式运行沿用整车例程的
`FRIC_DOWN=1520 us` 和 `FRIC_UP=1550 us`；这两个值是安全调参起点，实际转速、供弹能力和温升仍需台架确认。

## 调参入口

C615 的任务参数集中在 `Core/Inc/task/task_feed_motor/task_feed_motor_config.h`：
`FEED_MOTOR_SNAIL_STOP_PULSE_US=1000`、`MAX_PULSE_US=1550`、
独立测试默认先输出停止脉宽 3000 ms，再执行活动 Ramp；测试头文件中的
`SNAIL_2305_TEST_STARTUP_STOP_TIME_MS` 可调整等待时间。正式供弹仍使用
`ACTIVE_PULSE_US=1520`、`RAMP_TIME_MS=300`（这些宏带 `FEED_MOTOR_SNAIL_` 前缀）。
供弹运行时将这些参数组装为 `Snail2305_ConfigTypeDef`，斜率为
`(ACTIVE-STOP)*1000/RAMP_TIME_MS` us/s。活动脉宽必须大于停止脉宽且不超过最大脉宽。

BSP 只保留 `SNAIL_2305_MIN_PROTOCOL_PULSE_US=400` 与
`SNAIL_2305_MAX_PROTOCOL_PULSE_US=2200` 两个协议边界。减少 Ramp 时间会增加启动冲击；
增加活动脉宽会增加温升。当前正式上限 1550 us 来自整车例程；停止值改变时，还需在 `.ioc` 同步两个通道的初始 Pulse。
本轮默认停止值未改变，不需要修改 `.ioc`。

## C615 行程与转向校准

校准入口为 `bsp/snail_2305/test_snail_2305_calibration`，由
`FEED_MOTOR_SNAIL_MODE` 是唯一模式选择入口，定义在
`Core/Inc/task/task_feed_motor/task_feed_motor_config.h`：`0` 为正式供弹，`1` 为
PWM 行程校准，`2` 为电机转向切换。不要再修改 BSP 校准头文件选择模式。
启用校准时，任务启动后同时让 CH1/CH2 输出
`SNAIL_2305_CALIBRATION_MAX_PULSE_US`。模式 `1` 默认保持
`SNAIL_2305_CALIBRATION_MAX_HOLD_MS=2000 ms`，模式 `2` 默认保持 `4000 ms`，随后同时切换到
`SNAIL_2305_CALIBRATION_MIN_PULSE_US` 并保持。最小阶段结束后软件继续输出最小值，
不会自动恢复正式供弹。

操作步骤：

1. 断开拨弹机构，准备急停；确认 CH1/CH2 的 C615 信号线和电机供电正确。
2. 只启用校准模式，并根据目标修改最大/最小脉宽；不要同时启用 C610 自循环或正式供弹。
3. 让电调上电并运行程序。程序先输出最大脉宽；在手册的 BB 窗口使用模式 `1`，在 BBB 窗口使用模式 `2`。
4. 模式 `1` 约 2 秒、模式 `2` 约 4 秒后输出最小脉宽。由于 MCU 不能听到 BB/BBB，固定切换时刻不能保证与声音窗口完全同步；应根据实测声音调整 `SNAIL_2305_CALIBRATION_MAX_HOLD_MS`，并观察约 1 秒 B 声后保持到日志提示“时序完成”，随后断电确认。
5. PWM 行程校准后，将实测停止和最大脉宽写回 `FEED_MOTOR_SNAIL_STOP_PULSE_US`、
   `FEED_MOTOR_SNAIL_MAX_PULSE_US`；方向切换后确认两路物理转向，再设置正式 CH1/CH2 方向宏。

MCU 不能读取 C615 的蜂鸣器声音，也不能判断电调是否真正保存了校准结果；
`时序完成`只表示软件已经保持最小脉宽。实际行程、方向、鸣音和温升必须由台架确认。
