# 供弹电机任务

供弹 M2006 使用 CAN2 C610 ID1（反馈 `0x201`、控制 `0x200`），与 Yaw/Pitch 共用云台总线；正式与只读角度测试均绑定 `hcan2`。CAN1 预留底盘，C615 仍使用 TIM1 PWM。

`task_feed_motor` 管理唯一 C610/M2006 和两个 C615。DBUS 每 2 ms 发布一份带接收时间戳的命令快照；供弹任务在任务上下文读取快照、复制 CAN 反馈、更新步长控制、推进 PWM Ramp，再提交 C610 聚合帧。命令或反馈超过 100 ms 时两个输出进入安全停止。

## 输入语义

- 鼠标左键：按下沿建立一个 `FEED_MOTOR_STEP_COUNTS` 目标。松开不会取消未完成步长；目标达到后停止。
- 鼠标左键短按：完成一个步长。
- 鼠标左键长按：重复“转一个设定步长 → 零电流停顿 200 ms → 下一步”。每步到位周期必须返回零。下一步同时要求长按已满 400 ms、当前步到位后的停顿已满 200 ms，且左键仍按住；按下前的空闲时间不计入长按。
- 停顿中松开左键：取消后续重复并进入 STOP；正在转动时松开则完成当前目标后停止。
- 键盘 Shift（DBUS keyboard bit4）：只控制 C615。按住时两路 PWM Ramp 到活动脉宽，松开当周期直接写停止脉宽并重置 Ramp。
- Shift 与鼠标左键相互独立，可以只转摩擦轮，也可以在摩擦轮运行时进行步长供弹。

## 状态机和算法

```text
STOP --左键按下沿--> STEP --目标达到且仍按住，立即零电流--> WAIT_NEXT_STEP
WAIT_NEXT_STEP --长按阈值达到、停顿结束且仍按住--> STEP（下一步）
STEP --目标达到且左键释放--> STOP
WAIT_NEXT_STEP --左键释放--> STOP
SHIFT：按住 -> C615 活动 PWM；松开 -> C615 停止 PWM
```

每次建立目标时使用当前连续反馈位置：

```c
target_count = angle_count + FEED_MOTOR_STEP_COUNTS;
```

步长为正时，`angle_count >= target_count` 结束；步长为负时，`angle_count <= target_count` 结束。连续角度由 C610 驱动展开，避免 0/8191 单圈回绕破坏比较。目标加法先检查 `int64_t` 边界，无法表示时清零并停止。

目标完成时，仍按住则进入 `WAIT_NEXT_STEP`，记录 `step_finished_tick=now_tick` 并返回零电流。运行时据此关闭 C610 输出许可、写零电流并提交 CAN；等待通过周期状态机推进，不调用阻塞延时。只有 `now_tick-press_start_tick >= pdMS_TO_TICKS(CONTINUOUS_PRESS_MS)` 和 `now_tick-step_finished_tick >= pdMS_TO_TICKS(STEP_PAUSE_MS)` 同时满足才用最新反馈建立下一目标。停顿期间惯性移动不会提前建立新目标，也不会改变到位计时。

电流为 `sign(STEP_COUNTS) × FEED_CURRENT_RAW × CURRENT_SIGN`。控制器不使用停滞检测、预旋或单发摩擦轮保持。任务正在完成目标时不排队额外短按；通信安全门关闭会清除目标与停顿状态。供弹停顿不停止仍由 Shift 请求的摩擦轮。

## 文件职责

```text
task_feed_motor_config.h    # 步长、电流、Shift 摩擦轮和 PWM 参数
task_feed_motor_command.h   # DBUS 命令邮箱（左键、Shift、时间戳）
task_feed_motor_control.*   # 非阻塞固定步长状态机
task_feed_motor_runtime.*   # C610/C615、反馈快照、PWM、CAN 和日志
task_feed_motor.c           # 2 ms FreeRTOS 入口；测试分支与正式模式互斥
```

## 主要配置

| 宏 | 默认值 | 作用 |
|---|---:|---|
| `FEED_MOTOR_STEP_COUNTS` | `+4000 count` | 沿用当前源码单发配置，非零；符号同时决定目标和逻辑电流方向 |
| `FEED_MOTOR_CONTINUOUS_PRESS_MS` | `400 ms` | 长按连续步长阈值；短按完成一步后等待松键 |
| `FEED_MOTOR_STEP_PAUSE_MS` | `200 ms` | 用户确认的步间零电流停顿；范围 (0,INT32_MAX] 且至少 1 Tick，增大降低重复频率；从每步到位计时 |
| `FEED_MOTOR_FEED_CURRENT_RAW` | `5000 raw` | 沿用当前 C610 步长运行电流幅值，不改变现有调参 |
| `FEED_MOTOR_CURRENT_SIGN` | `+1` | 取 ±1；最终电流为步长符号 × 本符号 × 5000 raw |
| `FEED_MOTOR_FEEDBACK_SIGN` | `+1` | 反馈连续角度到逻辑坐标的符号 |
| `FEED_MOTOR_SNAIL_STOP_PULSE_US` | `1000 us` | Shift 松开或安全停机的 PWM |
| `FEED_MOTOR_SNAIL_ACTIVE_PULSE_US` | `1400 us` | 沿用当前源码 Shift PWM 目标；有效范围 (1000,1550] us，增大会增大油门与温升 |
| `FEED_MOTOR_SNAIL_RAMP_TIME_MS` | `10 ms` | 正数；仅 C615 启动 Ramp 时长，减小会增大启动冲击，松键停止绕过 Ramp |

步长、方向和 PWM 数值仍需上板确认。当前 +4000 count 对应电机转子约 +175.78°；拨弹盘角度还需除以实际减速比。状态机还会比较本步实际位移；反馈方向配反时达到同等位移也会结束，但不能把反向移动当作正确供弹。固定电流在目标处清零不提供主动位置保持；200 ms 表示零电流等待时长，实际惯性超调与停转时间须实测。PWM 停止值写入后，TIM1 CCR 预装载最迟下一 20 ms 周期生效。

## 独立测试

`FEED_MOTOR_ANGLE_STEP_TEST_ENABLE=1` 时运行 C610 只读手动角度测试，C610 始终输出零电流；`SNAIL_2305_TEST_ENABLE=1` 时运行独立双 PWM 测试。两个测试互斥，完成后恢复为 0。

## 验证

本轮停顿回归直接编译正式控制源码：1000 Hz/500 Hz Tick × +4000/-700 count 四组，各 7120 项检查通过。覆盖 100 个起点各 10 发、每次到位零电流、停顿截止前后、Tick 回绕、停顿中松键/故障、停顿中的惯性位移、长按阈值及短按完成；默认和日志开启 Debug 构建通过。此前 CAN、DBUS 与 PWM 回归见 Bug 记录。临时测试验证后删除；实际机械停转与超调待上板确认。
