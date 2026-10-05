# 供弹电机任务

## 目标与边界

供弹任务只管理本项目唯一的 M2006 供弹电机，默认 C610 电调 ID 为 1，反馈帧为 `0x201`。硬件调参模式由驱动同级的 C610 自循环函数负责上弹/停止/下弹/停止；正式路径等待真实供弹命令接入，没有凭空加入未经台架标定的堵转算法。

任务结构与 Yaw 保持一致：入口只做周期调度，硬件测试函数负责调参阶段，命令/控制文件保留正式命令接入边界，运行时组合 C610 和日志。关闭硬件测试后，当前因尚未接入真实供弹命令而保持零输出。

## 文件职责

```text
Core/Inc/task/task_feed_motor/task_feed_motor.h
Core/Src/task/task_feed_motor/task_feed_motor.c             # 入口和 2 ms 调度
Core/Inc/task/task_feed_motor/task_feed_motor_command.h
Core/Src/task/task_feed_motor/task_feed_motor_command.c     # 正式命令默认配置
Core/Inc/task/task_feed_motor/task_feed_motor_control.h
Core/Src/task/task_feed_motor/task_feed_motor_control.c     # 阶段状态机和目标电流
Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h
Core/Src/task/task_feed_motor/task_feed_motor_runtime.c     # 驱动/控制组合
```

## 状态流程

```text
WAIT_FEEDBACK
    ↓ 收到真实 0x201 反馈
UP (+700)
    ↓ 500 ms
STOP_AFTER_UP (0)
    ↓ 500 ms
DOWN (-500)
    ↓ 500 ms
STOP_AFTER_DOWN (0)
    └────────────── 回到 UP
```

停止阶段用于释放惯性和齿轮间隙，避免正负电流瞬时反向。所有电流每 2 ms 聚合发送一次，反馈超时后回到 `WAIT_FEEDBACK` 并持续发送零电流。

## 重要 Bug 修复

旧实现只用 `state != OFFLINE` 判断在线。初始化时 `last_feedback_tick` 为 0，如果系统启动后的前 100 ms 内还没有收到 CAN 帧，时间差可能仍小于超时阈值，导致驱动错误报告 `online=1`。任务因此可能把“尚未收到反馈”误判成已在线，日志表现为 `online=1 phase=wait_feedback` 或提前使能。

现在 C610 句柄增加 `feedback_received`：只有收到至少一帧合法反馈并且没有超时才算在线；超时同时关闭输出。日志增加 `feedback=0/1` 和 `age_ms`，可以区分“从未收到反馈”和“收到过但已掉线”。

## 时间基准

- `HAL_GetTick()`：CAN ISR 写入反馈时间，运行时传给 `C610_M2006_Process()` 判断超时。
- FreeRTOS Tick：阶段起点、阶段持续时间和日志限频。配置结构里的 `*_time_ms`
  和 `log_period_ms` 面向调试人员使用毫秒保存；控制层和运行时在比较前分别用
  `pdMS_TO_TICKS()` 转成 Tick，不能把两个单位混在一次减法中。
- `vTaskDelayUntil()`：任务固定 2 ms 唤醒周期。

两种时间不能直接相减。当前 FreeRTOS 配置为 1 kHz，只是转换结果恰好通常等于
毫秒数；如果以后修改 `configTICK_RATE_HZ`，阶段和日志仍应保持真实的毫秒语义。
阶段时间使用 `now_tick - phase_start_tick` 的无符号差值，因此短时间间隔跨过 Tick
回绕也能正常比较；不要把 `HAL_GetTick()` 的时间戳写入阶段起点。

## 各阶段的意义

- `WAIT_FEEDBACK`：尚未收到合法 `0x201` 反馈，确认 CAN 接收、ID 和驱动注册前强制零输出。
- `UP`：用较小的正向原始电流验证上弹方向；阶段起点在第一帧有效反馈到达后记录。
- `STOP_AFTER_UP`：上弹后短暂卸力，给惯性和齿轮间隙留出缓冲，避免马上反向顶齿。
- `DOWN`：用较小的负向原始电流验证下弹方向，持续时间可独立调整。
- `STOP_AFTER_DOWN`：下弹后再次归零，然后从 `UP` 开始下一轮；单独命名便于日志定位换向阶段。

任务每 2 ms 刷新一次聚合 CAN 控制帧，阶段计时、日志限频和任务唤醒分别使用各自
的 Tick 变量。这样日志不会挤占控制周期，阶段切换也不会因为串口发送耗时而漂移。

## 参数调整

默认值集中在 `task_feed_motor_command.c`：

- 电机 ID：`1`
- 上弹电流：`+700`
- 下弹电流：`-500`
- 上弹/下弹时间：`500 ms`
- 换向停止时间：`500 ms`
- 日志间隔：`500 ms`

这些是低电流测试起点。实机上方向相反时只调整上、下弹电流的符号；第一次上电必须有人值守并准备急停，不能把测试值当作最终供弹参数。

## 日志

日志在任务上下文限频输出：

```text
[供弹] 在线=1 反馈有效=1 阶段=上弹 角度=... 转速=...
      反馈电流=... 目标电流=... 输出=1 反馈年龄=...
```

`feedback=0` 表示从未收到合法反馈；`feedback=1 online=0` 表示曾经收到反馈但当前已经超时。这样不会再把初始化阶段的零时间戳误认为真实在线。

日志、控制和 CAN 接收的上下文必须分开：`HAL_CAN_RxFifo0MsgPendingCallback()` 只负责
把反馈帧交给 C610 驱动，不能在 ISR 中格式化字符串或等待 UART；`FeedMotor_RuntimeRunCycle()`
在任务上下文里读取反馈、更新阶段、发送聚合帧并限频打印。`C610_M2006_SendAll()`
每轮刷新目标电流，因为电调不会永久保存上一帧的控制意图。

## 接入前提与文件边界

- CAN1 已由 CubeMX 初始化、启动并打开 FIFO0 pending 中断，过滤器能够接收 `0x201`。
- USART1 及其 TX DMA 已初始化，`usart_printf()` 可在任务上下文发送诊断文本。
- C610 电调 ID 与 `FeedMotorCommand_GetDefault()` 返回的 `motor_id` 一致；当前默认值是 1。
- 机械结构允许正、反两个方向运动，首次上电必须有人值守并准备断电或急停。
- `freertos.c` 中的 `__weak task_feed_motor_entry()` 只做安全兜底；目录中的强定义由顶层
  `CMakeLists.txt` 显式加入后覆盖它。重新生成 CubeMX 文件时不要把业务状态机放回生成文件。

配置、阶段状态机和硬件组合分别通过公开头文件提供接口；反馈新鲜度由 C610 驱动层负责，其它任务不得直接修改
`FeedMotor_RuntimeTypeDef` 内部字段。以后接入遥控器时，应在命令层增加输入适配，不要把
遥控协议和 C610 CAN 帧解析混在任务入口。

## 验证

静态检查应确认入口、命令、控制和运行时源文件全部加入 CMake；ARM GCC `-Wall -Wextra -fsyntax-only` 可验证接口和类型。上弹方向、电流安全性、机械卡弹和温升必须在可急停台架实测。

## 硬件调参测试

供弹任务只保留驱动同级的
`bsp/c610_m2006/test_c610_m2006_self_cycle.*`。将
`C610_M2006_HARDWARE_TEST_ENABLE` 设为 1 后，任务入口直接运行上弹、停止、下弹、
停止自循环；用户只需修改头文件中的阶段时间和原始电流。函数每次调用推进一个非阻塞
步骤，反馈无效时强制零输出。调参完成后设为 0，进入正式供弹运行时；协议和算法边界
验证不再建立永久测试文件。

默认命令配置直接复用硬件测试头文件中的阶段和电流宏，后续正式命令接入时沿用已确认
的调参值；当前正式路径因为尚未接入真实供弹命令仍保持零输出，反馈超时也由 C610 驱动层清零。
