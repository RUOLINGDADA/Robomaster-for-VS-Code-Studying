# Yaw 水平轴任务

## 入口与配置

`task_yaw.c` 只做初始化、2 ms 周期调度和一个模式调用；`task_yaw_runtime.c`
只适配本轴配置与命令来源。正式控制和诊断实现位于 `app/gimbal/`。
CubeMX 入口仍是 `task_yaw_entry`，强定义由顶层 CMake 链接，不修改 .ioc 创建方式。

用户修改 `Core/Inc/task/task_yaw/task_yaw_config.h`：

| 参数 | 含义 |
| --- | --- |
| `YAW_HARDWARE_TEST_MODE` | 当前默认 OFF：摇杆与鼠标正式命令；CALIBRATION 只读标定；ANGLE_LOOP 固定目标 |
| `YAW_CALIBRATION_VALID` | 当前为 1，使用用户填写的中心与安全边界；重新标定前设 0 |
| `YAW_CALIBRATION_CENTER/MIN/MAX_ANGLE_RAW` | 中位、较小、较大连续计数；不能假定左边数值小 |
| `YAW_ANGLE_LOOP_TARGET_ANGLE_DEG` | 相对中位目标，默认 +30°，可填正负小数 |
| `YAW_POSITION_KP.../SPEED_KP.../SPEED_KI...` | 正式与测试共享位置/速度 PID 参数；当前位置 I/D=0，速度 D=1.5 |
| `YAW_SPEED_KD_CURRENT_S_PER_RPM` | 速度 D，原始值·s/rpm，当前 1.5；误差差分会放大噪声及目标突变 |
| `YAW_SPEED_INTEGRAL_LIMIT_RAW` | 积分项独立上限，默认等于总电流上限，允许 0 |
| `YAW_VELOCITY_FEEDFORWARD_GAIN` | 命令速度前馈，无量纲，当前 1.2；固定角度速度为 0，前馈为 0 |
| `YAW_SPEED_FILTER_ALPHA` | 测速低通本次采样权重，范围 (0,1] |
| `YAW_TASK_PERIOD_MS` | 测试与正式共用控制周期，默认 2 ms，编译时拒绝不足一个 Tick 的周期 |
| `YAW_FEEDBACK_TIMEOUT_MS/COMMAND_TIMEOUT_MS` | CAN 反馈及正式输入命令超时，默认均为 HAL 100 ms |
| `YAW_TASK_LOG_PERIOD_MS` | 正式诊断间隔，默认 500 ms；测试日志仍由 ANGLE_LOOP_LOG_PERIOD_MS 控制 |
| `YAW_MAX_CURRENT_RAW/CURRENT_SLEW_RAW_PER_S` | 电流上限及 Ramp，原始值和原始值/秒 |

## 调参步骤

1. 初次标定时选择 CALIBRATION、VALID=0，烧录后始终零电流，按配置间隔提示记录中心及边界。
2. 用户手动记录三组连续计数，给两侧留机械余量，排序写入 min/max，再确认有效标志。
3. 切换 ANGLE_LOOP、修改目标度数，观察到位、保持和拨离后回到目标；调整本轴配置的 PID、Ramp 和限流。
4. 切换 OFF，正式模式使用同一个轴实例、标定与控制参数，仅改变输入来源。

### 保留测试并调整正式参数

所有控制参数都由 `YawTask_RuntimeInit()` 复制到同一个 `GimbalAxis`，没有另一份正式 PID。
当前配置为位置 P=0.01 rpm/count、I/D=0；速度 P=200 raw/rpm、I=20 raw/(rpm·s)、
D=1.5 raw·s/rpm；前馈系数 1.2，积分上限等于 7000 raw 电流上限。
速度和电流滤波均使用 0.20 权重。参数以配置头为准，稳定性仍须上板确认。

运动跟随落后且没有速度/电流饱和时，可小步增加前馈，超前或过冲则减小；前馈只改变送给
速度环的请求，目标角度仍按原命令速度积分。固定角度测试的命令速度为零，所以调整前馈
不会改变 30°到位行为，也不能用这个静止目标验证运动前馈效果。

积分补偿过大时可降低积分上限；持续静差时先看积分是否已顶住，再调 Ki。速度 D 当前为 1.5 raw·s/rpm，
当前使用滤波后速度误差差分，目标突变也会产生 D 输出，没有另加微分专用滤波。
重力、摩擦及加速度前馈尚未实现，不提供没有实际作用的配置开关。

### 位置 P 增大却没更快、目标附近抖动

当前位置 I/D=0 时，固定目标的速度请求为 `clamp(位置 P × 角度误差, ±速度上限)`，角度误差单位是编码器计数，
不是度；8192 计数对应 360°。若速度上限为 40 rpm，位置 P=10 rpm/计数时，差 4 计数
（约 0.18°）就会请求满速，目标附近的渐进减速区很窄。继续增加 P 不会提高限幅后的速度，
却可能加重反复换向。P=0.06 时，差 1°请求约 1.37 rpm，差 30°请求约 40.96 rpm。

先把位置 P 恢复到合理量级。动作慢时，看目标速度是否饱和、实际速度是否跟得上，以及电流
是否饱和；这三种情况应分别处理。目标附近速度请求未饱和且实际速度能跟随时，可按 0.01
小步增加位置 P；大幅摆动时不能同时升速度上限、位置 P 和积分。速度 P 也提供制动，
减小它不一定能消除摆动。电流 Ramp 越慢，启动越柔和，但反向刹车也越迟。

这些计算用于定位限幅与单位问题，不能证明某组增益适合实际机械负载；保持抖动还需用当前
固件日志核对误差、速度、电流及输出许可，避免把旧参数日志当作新参数实测结果。

## 正式输入与控制

适配层提交 `YawCommand_Submit()`：速度意图 -1000~1000、enabled 和 `HAL_GetTick()` 时间戳。
命令超过 100 ms 未刷新变为中立；非零命令积分改变目标，松手保持最后目标。数据流是
相对速度命令 → 目标积分与范围限制 → 位置 PID 与速度前馈 → 限幅速度 → 速度 PID → 电流 Ramp → CAN。
当前位置 I/D 关闭，速度 D 已启用。正式输入叠加 DBUS 右摇杆左右 CH0 与鼠标 X，不做 IMU 世界系稳定。

## 保护与日志

软件边界只过滤继续向外的输入，回中或命令超时按零速度保持最后目标位置，反向命令同周期积分目标；反馈掉线仍发送零电流。Yaw 使用独立重力算法对象，`YAW_GRAVITY_COMPENSATION_ENABLE=0` 时输出恒为 0 raw。

每周期只取一次 GM6020 快照。日志含计算电流、保护后目标电流、输出许可和 CAN 提交结果，
明确区分命令与实际反馈；串口忙时非阻塞重试。误差单位计数；目标偏角显示到 0.01°，已按
8192 计数/圈量化（单计数约 0.04395°）；目标速度为百分之一 rpm，避免依赖 printf 浮点支持。

## 验证与限制

已用 ARM GCC 静态检查实际 C 驱动和控制链；边界行为、方向和温升仍需上板实测。
默认标定、角度测试和正式宏分支静态验证；机械边界、转向、PID 和温升仍需上板实测。

设计思路参考公开 RoboMaster PID/Ramp 分层，没有复制与当前配置无关的控制代码：

- [PID.c](https://github.com/CuboiLeo/RoboMaster_Mecanum_Standard/blob/main/Algorithms/Algorithms.c/PID.c?plain=1)
- [Ramp_Calc.c](https://github.com/CuboiLeo/RoboMaster_Mecanum_Standard/blob/main/Algorithms/Algorithms.c/Ramp_Calc.c?plain=1)
