# 供弹电机任务

`task_feed_motor` 管理唯一 C610/M2006 供弹电机和两个 C615 摩擦轮。任务不创建新的
FreeRTOS 对象。DBUS 任务发布鼠标左键命令，供弹任务读取带 HAL 毫秒时间戳的命令快照，
并在同一周期完成 PWM、供弹状态机、CAN 聚合发送和日志。

正式路径不实现卡弹锁存、停滞超时故障、热量限制或弹丸计数。单发使用连续反馈位置停滞
确认决定何时停止上弹并开始单发保持；摩擦轮在上弹前预旋。这不是机械卡弹诊断。命令超时或反馈失效时 M2006 立即清零，
C615 立即写停止脉宽。短按释放不会中断当前单发；连发释放当周期清零 M2006 电流，C615 目标转为停止脉宽并按 Ramp 减速。

## 文件职责

```text
Core/Inc/task/task_feed_motor/task_feed_motor_config.h    # 电流、停滞、按键和 PWM 参数
Core/Inc/task/task_feed_motor/task_feed_motor_command.h   # DBUS 命令邮箱接口
Core/Src/task/task_feed_motor/task_feed_motor_command.c   # 命令快照和 100 ms 超时
Core/Inc/task/task_feed_motor/task_feed_motor_control.h   # 单发/连发状态和停滞算法
Core/Src/task/task_feed_motor/task_feed_motor_control.c   # 非阻塞状态机
Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h   # C615/C610 硬件组合接口
Core/Src/task/task_feed_motor/task_feed_motor_runtime.c   # 反馈快照、PWM、CAN 和日志
Core/Src/task/task_feed_motor/task_feed_motor.c           # 2 ms FreeRTOS 调度入口
```

## 状态机

```text
STOP
  └─ 左键按下沿且 DBUS/CAN 安全门有效 → FRIC_SPINUP
FRIC_SPINUP（首发预旋）
  └─ 双 C615 达到活动 PWM → FEED
FEED
  └─ C615 保持活动；位置变化 ≤ 20 count → FEED_SETTLE
FEED_SETTLE
  ├─ C615 保持活动；位置重新变化 → FEED
  └─ 停滞连续 100 ms → SINGLE_FIRE
SINGLE_FIRE
  └─ 保持 150 ms → WAIT_RELEASE 或 CONTINUOUS_FEED
WAIT_RELEASE
  ├─ 松键 → STOP
  └─ 长按阈值达到且仍按住 → CONTINUOUS_FEED
CONTINUOUS_FEED
  └─ C610 与 C615 同时运行；松键/超时/反馈失效 → STOP
```

单发先让两路 C615 预旋到活动 PWM，再驱动 C610 上弹；C610 上弹和停滞确认期间继续保持摩擦轮活动。
长按阈值只设置 `continuous_requested`，不能打断第一发；
第一发保持完成后才进入 `CONTINUOUS_FEED`。正式路径不再使用 `FEED_MOTOR_STEP_COUNTS`、
位置 P 或每发角度目标。

## 算法和时序

首发先进入 `FRIC_SPINUP`，两路 `pulse_us` 达到活动值后才进入 `FEED`。单发 `FEED`/`FEED_SETTLE`
每个 2 ms 周期同时保持 C610 供弹电流和 C615 活动 PWM：

```text
delta_count = angle_count - last_position_count
abs(delta_count) > FEED_MOTOR_STALL_COUNTS
    → 记录新位置，清零停滞确认
否则
    → 累加 stall_elapsed_ms
```

位置连续不超过 `20 count` 达到 `100 ms` 后，停止 C610 并进入 `SINGLE_FIRE`。摩擦轮已经在上弹前预旋；若位置重新
变化，回到 `FEED` 并重新确认。当前按需求不增加停滞超时故障，所以卡弹或传感器异常必须
通过急停台架观察。

进入首发 `FRIC_SPINUP` 后，两路 C615 通过正式 Ramp 到活动脉宽；两个 `pulse_us` 都达到活动值
后才允许 C610 上弹。C615 没有转速反馈，CCR 到目标只表示 PWM 已写入，
不等于摩擦轮真实转速稳定。

停滞确认结束后，C610 当周期清零，两路 C615 保持活动 PWM `150 ms`。
首发预旋使用现有 `FEED_MOTOR_SNAIL_RAMP_TIME_MS=300 ms`；当前不增加额外转速稳定等待。

左键按下持续达到 `FEED_MOTOR_CONTINUOUS_PRESS_MS` 后，只设置连发意图。若当前单发尚未
完成，仍按单发顺序执行；单发保持结束且左键仍按住时，进入 `CONTINUOUS_FEED`，C610 固定
电流和 C615 活动 PWM 同时保持。连发不调用停滞判定。

## 公开配置

| 宏 | 单位/协议 | 默认值 | 调整影响 |
|---|---|---:|---|
| `FEED_MOTOR_TASK_PERIOD_MS` | 任务周期 ms | 2 | 影响反馈采样和 PWM Ramp 推进 |
| `FEED_MOTOR_COMMAND_TIMEOUT_MS` | DBUS 命令期限 ms | 100 | 过期立即停止两个电机 |
| `FEED_MOTOR_STALL_COUNTS` | 停滞位置窗口 count | 20 | 增大可能提前判定转不动 |
| `FEED_MOTOR_STALL_CONFIRM_MS` | 停滞确认 ms | 100 | 减小会增加未到位误判 |
| `FEED_MOTOR_SINGLE_FIRE_HOLD_MS` | 单发摩擦轮保持 ms | 150 | 过短可能夹弹未出，过长增加空转 |
| `FEED_MOTOR_CONTINUOUS_PRESS_MS` | 长按阈值 ms | 400 | 达到后首发完成再进入连发 |
| `FEED_MOTOR_FEED_CURRENT_RAW` | M2006 供弹电流 raw | 700 | 增大力矩和温升 |
| `FEED_MOTOR_MAX_CURRENT_RAW` | C610 电流上限 raw | 700 | 供弹电流安全上限，不超过 10000 |
| `FEED_MOTOR_FEEDBACK_SIGN` | 角度/速度符号 | +1 | 改变停滞日志和连续角度方向 |
| `FEED_MOTOR_CURRENT_SIGN` | 逻辑电流符号 | +1 | 改变 M2006 实际转动方向 |
| `FEED_MOTOR_SNAIL_STOP_PULSE_US` | C615 停止脉宽 us | 1000 | 故障和停止路径直接写入 |
| `FEED_MOTOR_SNAIL_ACTIVE_PULSE_US` | C615 活动脉宽 us | 1520 | 单发/连发摩擦轮目标；沿用参考工程 `FRIC_DOWN` |
| `FEED_MOTOR_SNAIL_MAX_PULSE_US` | C615 上限 us | 1550 | 必须不小于活动值 |
| `FEED_MOTOR_SNAIL_RAMP_TIME_MS` | PWM Ramp ms | 300 | 过短增加启动冲击 |

所有数值都是待台架验证的起点。必须在可急停台架上确认 M2006 方向、停滞误判、C615 转向、
单发保持时间、供弹能力和温升。

## 独立硬件测试

### C615 摩擦轮独立测试

`Core/Inc/bsp/snail_2305/test_snail_2305.h` 的 `SNAIL_2305_TEST_ENABLE` 显式开启后，
只初始化两路 PWM，不注册 C610、不读取 DBUS。测试完成后恢复为 `0`。

### C610 手动角度反馈测试

`FEED_MOTOR_ANGLE_STEP_TEST_ENABLE=1` 后，只注册 C610、不启动 C615、不读取 DBUS，并始终
提交零电流。首帧建立 `reference_count`，日志输出当前连续角度、相对基准和采样增量。
该测试只确认反馈方向和连续角度，不再为正式控制提供步长参数。完成后必须恢复开关为 `0`。

## 验证

软件验证应覆盖 DBUS 命令超时、短按/长按、停滞确认、单发摩擦轮顺序、单发保持、连发释放、
反馈掉线和 C615 Ramp。默认正式构建、角度测试构建和 C615 独立测试构建均需通过 CMake；
软件构建不能代替方向、卡弹、供弹能力和温升台架验证。
