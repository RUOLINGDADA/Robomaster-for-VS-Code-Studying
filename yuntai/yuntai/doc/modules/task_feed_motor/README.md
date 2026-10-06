# 供弹电机任务

## 目标与边界

`task_feed_motor` 管理唯一的 C610/M2006 供弹电机和两个 C615 摩擦轮。任务不创建新的
FreeRTOS 对象。DBUS 任务发布鼠标左键命令，供弹任务读取带 HAL 毫秒时间戳的命令快照，
并在同一周期完成 PWM、角度控制和 CAN 聚合发送。

正式路径不实现堵转检测、热量限制或弹丸计数。C610 反馈必须新鲜，DBUS 必须在线；任一
安全条件失败时，M2006 电流为零，C615 立即写入 1000 us。左键释放且通信仍在线时，
C615 使用 300 ms 斜坡返回停止值。

## 文件职责

```text
Core/Inc/task/task_feed_motor/task_feed_motor_config.h    # 角度、时序和电流配置
Core/Inc/task/task_feed_motor/task_feed_motor_command.h   # DBUS 命令邮箱接口
Core/Src/task/task_feed_motor/task_feed_motor_command.c   # 命令快照和 100 ms 超时
Core/Inc/task/task_feed_motor/task_feed_motor_control.h   # 预旋、步进和间隔状态
Core/Src/task/task_feed_motor/task_feed_motor_control.c   # P 控制与到位确认
Core/Inc/task/task_feed_motor/task_feed_motor_runtime.h   # 硬件组合接口
Core/Src/task/task_feed_motor/task_feed_motor_runtime.c   # C615、C610 和日志组合
Core/Src/task/task_feed_motor/task_feed_motor.c            # 2 ms FreeRTOS 调度入口
```

## 状态机

```text
STOP
  └─ DBUS 在线且左键按下 → SPINUP
SPINUP
  └─ 按下起经过 300 ms 且双 C615 到达 1800 us → ADVANCE
ADVANCE
  ├─ 目标误差 ≤ 65 count 且 |速度| ≤ 10 rpm，连续 20 ms → INTERVAL
  └─ 其它情况 → P 控制输出
INTERVAL
  └─ 等待 100 ms → ADVANCE（以已完成目标为下一发起点）
任一状态
  └─ DBUS 离线、命令过期、反馈过期、初始化失败或左键释放 → STOP
```

预旋时间从左键命令生效开始计算。两个 PWM 脉宽都到达活动值后，才允许 M2006 进入
第一步。下一发使用上一发的目标角度加 409 count；目标误差在状态切换后同一周期重新
计算，避免使用上一发误差产生短暂的错误电流。

## 输入与时间

- 命令邮箱由 `task_dbus` 写入、`task_feed_motor` 读取。短临界区保护多个字段；邮箱只
  保存最新命令，不积压旧鼠标状态。
- 命令时间戳来自合法 DBUS DMA 帧接收时刻，100 ms 内没有新命令即失效。时间戳 0 ms
  仍可表示真实首帧。
- CAN 反馈时间戳由 `HAL_GetTick()` 写入和比较。阶段计时使用 FreeRTOS Tick，并在比较
  前通过 `pdMS_TO_TICKS()` 转换。两个时基不能混用。
- 任务使用 `vTaskDelayUntil()` 保持 2 ms 周期。PWM 不使用 TIM1 中断。

## 公开配置

| 宏 | 单位/协议 | 默认值 | 调整影响 |
|---|---|---:|---|
| `FEED_MOTOR_ID` | CAN1 电调 ID；反馈 `0x201`，控制 `0x200` | 1 | ID 错误会一直离线 |
| `FEED_MOTOR_STEP_COUNTS` | M2006 电机轴 count/发 | 409 | 增大步距会降低供弹频率并可能撞齿 |
| `FEED_MOTOR_WINDOW_COUNTS` | 位置误差 count | 65 | 增大可提前判定到位 |
| `FEED_MOTOR_READY_SPEED_RPM` | 电机轴 rpm | 10 | 增大会提高惯性误判 |
| `FEED_MOTOR_SETTLE_MS` | 到位确认 ms | 20 | 减小会增加噪声误判 |
| `FEED_MOTOR_SHOT_INTERVAL_MS` | 发射间隔 ms | 100 | 减小会提高供弹频率 |
| `FEED_MOTOR_SPINUP_MS` | 预旋 ms | 300 | 减小会在摩擦轮未稳时拨弹 |
| `FEED_MOTOR_POSITION_KP` | C610 raw/count | 2.0 | 增大会加快动作并增加过冲 |
| `FEED_MOTOR_MAX_CURRENT_RAW` | C610 raw | 700 | 增大会增加力矩和温升 |
| `FEED_MOTOR_FEEDBACK_SIGN` | 逻辑角度/速度符号，±1 | +1 | 方向错误会使误差闭环反向 |
| `FEED_MOTOR_SNAIL_STOP_PULSE_US` | PWM us | 1000 | 停转异常时重新校准 |
| `FEED_MOTOR_SNAIL_MAX_PULSE_US` | PWM us | 1550 | 正式运行上限；沿用整车例程 `FRIC_UP` |
| `FEED_MOTOR_SNAIL_ACTIVE_PULSE_US` | PWM us | 1520 | 常规活动值；沿用整车例程 `FRIC_DOWN` |
| `FEED_MOTOR_SNAIL_RAMP_TIME_MS` | ms | 300 | 减小会增加启动冲击 |
| `FEED_MOTOR_CURRENT_SIGN` | 逻辑电流符号，±1 | +1 | 方向错误会使 M2006 反向运动 |

所有数值都是待台架验证的起点。先确认 CAN ID、反馈符号和电流符号，再调整步长、窗口
和增益。必须在可急停台架上验证拨弹盘齿数、实际每发角度、方向、电流和温升。

## 独立硬件测试

### C615 摩擦轮独立测试

测试头文件为 `Core/Inc/bsp/snail_2305/test_snail_2305.h`。将其中
`SNAIL_2305_TEST_ENABLE` 改为 `1` 后重新编译。它是独立于
`FEED_MOTOR_SNAIL_MODE` 的 C615 测试开关；开启时必须保持 `FEED_MOTOR_SNAIL_MODE=0`。
该模式只初始化 PE9/TIM1_CH1
和 PE11/TIM1_CH2，不注册 C610，不要求 M2006 反馈，也不读取 DBUS 左键。
两路先输出停止脉宽 3000 ms，再从停止脉宽 Ramp 到活动脉宽，保持设定时间，再 Ramp 回停止值。
拆除拨弹机构，先低活动脉宽上电，并准备硬件急停。测试完成后恢复为 `0`。

测试用的停止值、活动值、最大值、Ramp、保持时间和日志周期都在该测试头文件中；
正式供弹仍由本任务的 C615 配置控制。该模式用于区分“PWM/接线问题”和“DBUS/CAN
安全门问题”，不能作为正式发射入口。

在任务配置中设置 `FEED_MOTOR_TEST_ENABLE=1` 后，入口改为驱动同级的
`C610_M2006_TestSelfCycle_Run()`。该路径只执行上弹、停止、下弹、停止的非阻塞自循环，
不与正式鼠标路径同时写电流。测试完成后恢复为 0。
`FEED_MOTOR_TEST_UP/STOP/DOWN_TIME_MS`、`UP/DOWN_CURRENT_RAW` 和 `LOG_PERIOD_MS` 也在
同一任务配置中。入口一次组装 `C610_M2006_TestConfigTypeDef` 并传入测试接口；测试驱动不读取 task 头。
时间必须为 1~INT32_MAX ms，测试电流必须在 C610 的 ±10000 raw 范围内。

## 验证

软件行为由临时 HAL 替身验证：鼠标解析、叠加限幅、命令超时、角度回绕、预旋、到位、
间隔、释放停机和反馈掉线均通过。Debug 构建使用 `cmake --preset Debug` 和
`cmake --build --preset Debug` 验证。C615 行程、方向、M2006 每发角度、卡弹和温升仍需
上板确认。
