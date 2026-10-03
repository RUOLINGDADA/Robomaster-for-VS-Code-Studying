# Yaw 水平轴任务

## 入口与配置

`task_yaw.c` 只做初始化、2 ms 周期调度和一个模式调用；`task_yaw_runtime.c`
只适配本轴配置与命令来源。正式控制、保护和诊断实现位于 `app/gimbal/`。
CubeMX 入口仍是 `task_yaw_entry`，强定义由顶层 CMake 链接，不修改 .ioc 创建方式。

用户修改 `Core/Inc/task/task_yaw/task_yaw_config.h`：

| 参数 | 含义 |
| --- | --- |
| `YAW_HARDWARE_TEST_MODE` | 默认 CALIBRATION；ANGLE_LOOP 固定角度调参；OFF 正式命令 |
| `YAW_CALIBRATION_VALID` | 默认 0，实测确认中心与两侧安全边界后改 1 |
| `YAW_CALIBRATION_CENTER/MIN/MAX_ANGLE_RAW` | 中位、较小、较大连续计数；不能假定左边数值小 |
| `YAW_ANGLE_LOOP_TARGET_ANGLE_DEG` | 相对中位目标，默认 +30°，可填正负小数 |
| `YAW_POSITION_KP.../SPEED_KP.../SPEED_KI...` | 正式与测试共享角度 P 和速度 PI 参数 |
| `YAW_MAX_CURRENT_RAW/CURRENT_SLEW_RAW_PER_S` | 电流上限及 Ramp，原始值和原始值/秒 |
| `YAW_RELEASE_POSITION_DELTA_RAW` | 固定目标反向移开距离，台架起点 114 计数 |

## 调参步骤

1. 默认标定烧录后始终零电流，串口首次显示中位、左限位、右限位说明，约 5 秒重提示。
2. 用户手动记录三组连续计数，给两侧留机械余量，排序写入 min/max，再确认有效标志。
3. 切换 ANGLE_LOOP、修改目标度数，观察到位、保持和拨离后回到目标；调整本轴配置的 PID、Ramp、限流和保护阈值。
4. 切换 OFF，正式模式使用同一个轴实例、标定与控制参数，仅改变输入来源。

## 正式输入与控制

适配层提交 `YawCommand_Submit()`：速度意图 -1000~1000、enabled 和 `HAL_GetTick()` 时间戳。
命令超过 100 ms 未刷新变为中立；非零命令积分改变目标，松手保持最后目标。数据流是
相对速度命令 → 目标积分与范围限制 → 角度 P → 限幅速度 → 速度 PI → 电流 Ramp → 共用保护 → CAN。
位置环不额外做 D 项；速度反馈已经来自电机。输入协议尚未接入，不做 IMU 世界系稳定。

## 保护与日志

启动/换向有宽限，堵转需同时具备足够目标和反馈电流、低速、连续位置变化不足、反馈新鲜，
持续超过检测时间。相对控制时同方向保持零输出，松手或反向且电流下降可释放并以当前角度重置保持点。
固定目标确认堵转后必须手动向反方向移开约 5°且电流下降，不能用测试内零命令释放；掉线不清保护。
软件边界阻止向外指令，允许向内释放；掉线恢复和保护解除均先经过一个零输出周期。

每周期只取一次 GM6020 快照。日志含计算电流、保护后目标电流、输出许可和 CAN 提交结果，
明确区分命令与实际反馈；串口忙时非阻塞重试。误差单位计数；目标偏角显示到 0.01°，已按
8192 计数/圈量化（单计数约 0.04395°）；目标速度为百分之一 rpm，避免依赖 printf 浮点支持。

## 验证与限制

已用临时主机程序运行实际 C 驱动、控制与保护验证目标/保持/Ramp、双轴隔离、掉线与堵转恢复。
默认标定、角度测试和正式宏分支构建验证；机械边界、转向和 PID/阈值仍需上板实测。

设计思路参考公开 RoboMaster PID/Ramp 分层，没有复制与当前配置无关的控制代码：

- [PID.c](https://github.com/CuboiLeo/RoboMaster_Mecanum_Standard/blob/main/Algorithms/Algorithms.c/PID.c?plain=1)
- [Ramp_Calc.c](https://github.com/CuboiLeo/RoboMaster_Mecanum_Standard/blob/main/Algorithms/Algorithms.c/Ramp_Calc.c?plain=1)
