# C615 与 Snail 2305 PWM 驱动

## 作用

`bsp/snail_2305` 把 TIM1 CH1/CH2 的比较值转换为 C615 PWM 脉宽。它只处理
TIM HAL、脉宽范围、停止值和非阻塞斜坡；不判断电机转速，不解析 DBUS，也不实现
发射状态机。

本工程使用 PE9/TIM1_CH1 和 PE11/TIM1_CH2。TIM1 计数频率为 1 MHz，周期为
19999，因此 PWM 周期为 20 ms（50 Hz）。C615 手册允许 400~2200 us、最高
500 Hz；本工程正式运行使用 1000~1550 us；协议允许范围为 400~2200 us。

## 接口

- `Snail2305_Init()`：检查 TIM1 实例、通道、PSC、ARR 和范围，先写停止脉宽再启动 PWM。
- `Snail2305_SetPulse()`：立即写脉宽，用于安全停机和初始化。
- `Snail2305_SetStopPulse()`：更新停止脉宽并立即写入 CCR，适合人工台架确认后的调参。
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
在线且左键按下时，单发先让两路 C615 Ramp 到活动值，再由 M2006 固定电流上弹；上弹及
连续位置停滞确认期间两路 C615 始终活动。停滞确认后 C610 清零，C615 保持单发配置时间。
左键长按阈值达到且第一发完成后，M2006 与两路 C615
同时进入连发。

DBUS 离线、M2006 反馈超时、初始化失败或左键释放时，M2006 发送零电流；C615
故障时直接写 1000 us，正常释放时按斜坡回到 1000 us。两个轮使用同一脉宽，
相反旋转方向由 Snail 相线或 DJI Assistant 设置完成。

## CubeMX 对接

`.ioc` 持久化 TIM1、PE9、PE11、PSC=167、ARR=19999 和两个初始 Pulse=1000。
CubeMX 重新生成后必须确认 `tim.c/h`、TIM1 MSP 和 `cmake/stm32cubemx/CMakeLists.txt`
仍存在。PWM 启动属于任务上下文，不需要 TIM1 中断或 FreeRTOS ISR API。

第一次上电前按 C615 手册完成 PWM 行程校准，并准备急停。正式运行当前以
`FEED_MOTOR_SNAIL_ACTIVE_PULSE_US=1520 us`、`MAX_PULSE_US=1550 us` 为调参起点，
对应参考工程的 `FRIC_DOWN/FRIC_UP`。实际转速、供弹能力和温升仍需台架确认。

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

## 校准边界

本工程已删除 C615 软件校准测试及其专用任务模式。
C615 的实际行程、方向、停止点和温升必须使用外部安全工具或人工台架流程确认，结果再人工写入
`task_feed_motor_config.h`；不能把删除的软件测试当成已完成硬件校准。
