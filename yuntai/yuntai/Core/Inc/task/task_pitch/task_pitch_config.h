/**
  ******************************************************************************
  * @file    task_pitch_config.h
  * @brief   Pitch 轴 GM6020 标定、角度环测试和正式模式配置。
  *
  * 参数独立于 Yaw；先用标定模式记录本轴中心及上下机械边界，再切换到
  * 角度环模式。未完成标定时任何模式都保持零输出。
  * 通俗理解：先把中位和上下安全边界测准，再允许 Pitch 闭环运动。
  ******************************************************************************
  */

#ifndef TASK_PITCH_CONFIG_H
#define TASK_PITCH_CONFIG_H /* 防止 Pitch 配置重复包含（避免宏和标定值重复定义）。 */

#define PITCH_HARDWARE_TEST_MODE_OFF 0U /* 关闭硬件测试，运行右摇杆上下正式控制（未标定或遥控掉线仍零输出）。 */
#define PITCH_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读标定，强制零电流（人手移动并记录角度）。 */
#define PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 复用正式闭环保持固定角度（测试不复制控制器）。 */
#ifndef PITCH_HARDWARE_TEST_MODE
#define PITCH_HARDWARE_TEST_MODE PITCH_HARDWARE_TEST_MODE_OFF /* 当前默认固定目标角度测试；接入右摇杆前改为 OFF，标定模式按需显式切换（掉线仍零输出）。 */
#endif

#define PITCH_CALIBRATION_VALID 1U /* 1 表示本轴三组角度已实机确认（否则闭环保持零输出）。 */
#define PITCH_TASK_PERIOD_MS 2U /* 测试与正式任务周期，单位 ms（PID/Ramp 的 dt 与调度共用，不可小于一个 FreeRTOS Tick）。 */
#define PITCH_COMMAND_TIMEOUT_MS 100U /* 正式命令期限，单位 HAL ms（旧帧不能继续推动目标，回中有效命令仍闭环保持）。 */
#define PITCH_TASK_LOG_PERIOD_MS 500U /* 正式周期诊断间隔，单位 HAL ms（事件优先，UART 忙时重试）。 */
#define PITCH_FEEDBACK_TIMEOUT_MS 100U /* CAN 反馈离线阈值，单位 HAL ms（变大会延迟掉线停机，不能用来掩盖通信故障）。 */
#define PITCH_CALIBRATION_CENTER_ANGLE_RAW 8753 /* 中位连续角度，单位编码器计数（本轴位置零点）。 */
#define PITCH_CALIBRATION_MIN_ANGLE_RAW 8136 /* 连续总角度中的较小实测边界，单位计数（应与物理下限一致或略留安全余量；重力使炮管贴住端点时仍需保留零输出）。 */
#define PITCH_CALIBRATION_MAX_ANGLE_RAW 9350 /* 连续总角度中的较大实测边界，单位计数（必须大于中位，避免把环形单圈值直接拿来做限位）。 */
#define PITCH_MOTOR_CURRENT_SIGN 1 /* 根据最新上板结果：物理正电流才能把 Pitch 从下限抬向中心，逻辑电流保持正号。 */
#define PITCH_FEEDBACK_SPEED_SIGN 1 /* 与 Yaw 一致：GM6020 原始转速应与连续编码器角度增量同向；实测角度增加时日志速度也必须为正。 */

/* Pitch 抗重力保持：原始电流单位，1500 只是台架起点；幅值先为 0，确认恒定保持后再按实机温升逐步标定。 */
#define PITCH_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW 7000.0f
#define PITCH_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW 0.0f

#define PITCH_MAX_COMMAND_SPEED_RPM 50.0f /* Pitch 正式输入满量程速度上限，单位 rpm（输入满量程的最大速度）。 */
/*
 * 控制配置与固定角度测试共用；只有完成本轴标定并切换 OFF 模式后才接受 DBUS 正式输入。
 * 速度目标 = 位置 P × 连续角度误差 + 位置 KD × 角度误差变化率 + 前馈系数 × 命令速度，随后限幅。
 * 运动跟随落后且未饱和时可小步增加前馈，超前/过冲则减小；固定目标速度为 0，前馈也为 0。
 * （前馈只帮助跟随运动目标。）
 */
#define PITCH_VELOCITY_FEEDFORWARD_GAIN 1.0f /* 命令速度前馈系数，无量纲、非负；0 关闭、1 保留原行为（当前固定目标不产生前馈）。 */
/*
 * 下面的位置/速度环起始值按官方 19.gimbal_task 的 Pitch 编码器级联 PID 换算。
 * 官方 Pitch 角度环为 KP=15、max_out=10 rad/s，速度环为 KP=2900、
 * KI=60；换到本项目后分别是 15×60/8192=0.1099 rpm/计数、
 * 10×60/(2π)≈95 rpm、2900×2π/60=303.7 原始值/rpm，
 * 以及 60×2π/60/0.001=6283.2 原始值/(rpm·s)。
 * 通俗理解：官方用“弧度/秒 + 每毫秒积分”，本项目用“编码器计数/rpm + dt_s”，
 * 必须先换单位；直接把 15、2900、60 填进本项目会让输出大得不对。
 */
#define PITCH_MAX_SPEED_TARGET_RPM 95.0f /* Pitch 保守速度上限，单位 rpm；先限制摆动能量，再逐步增加。 */
#define PITCH_POSITION_KP_RPM_PER_RAW 0.05f /* 位置 P 起点，单位 rpm/计数；误差 1000 计数只请求约 10 rpm。 */
#define PITCH_POSITION_KI_RPM_PER_RAW_S 1.6f /* 先关闭位置 I，单位 rpm/(计数·s)；抗重力由独立偏置承担，避免积分叠加造成摆动。 */
#define PITCH_POSITION_INTEGRAL_LIMIT_RPM 5.0f /* 位置 I 上限，单位 rpm；重新启用积分时的安全起点。 */
#define PITCH_POSITION_KD_RPM_S_PER_RAW 0.0f /* 先关闭位置 D，单位 rpm·s/计数；速度反馈已在内环提供阻尼。 */
#define PITCH_SPEED_KP_CURRENT_PER_RPM 200.0f /* 速度 P 起点，单位原始电流/rpm；降低测速噪声导致的电流翻转。 */
#define PITCH_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 官方 PITCH_SPEED_PID_KI=60 按 1 ms 离散积分换算，单位原始电流/(rpm·s)（补偿重力静差）。 */
#define PITCH_MAX_CURRENT_RAW 7000.0f /* 本项目实机安全电流起点；独立于官方 max_out=30000，且必须不超过 GM6020 协议 16384。 */
#define PITCH_SPEED_KD_CURRENT_S_PER_RPM 0.0f /* 速度 D，单位原始值·s/rpm；默认关闭（误差差分会放大测速噪声和目标跳变，不能盲目加大）。 */
#define PITCH_SPEED_INTEGRAL_LIMIT_RAW PITCH_MAX_CURRENT_RAW /* 积分项绝对值上限，单位原始值，范围 0~MAX_CURRENT（补偿过大可降低，静差时先看积分是否已到上限）。 */
#define PITCH_CURRENT_SLEW_RAW_PER_S 60000.0f /* 电流变化率上限，单位原始值/秒；降低反向冲击。 */
#define PITCH_SPEED_FILTER_ALPHA 0.20f /* 速度低通本次采样权重，范围 (0, 1]；抑制 GM6020 速度量化噪声。 */


#define PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG 0.0f /* 固定目标相对中位角度，单位度，正负和小数均可，须在边界内（越界直接零输出）。 */
#define PITCH_ANGLE_LOOP_LOG_PERIOD_MS 200U /* Pitch 角度测试日志间隔，单位 ms（避免串口刷屏）。 */

#define PITCH_CALIBRATION_LOG_PERIOD_MS 200U /* 标定数据日志间隔，单位 ms（观察连续角度和反馈）。 */
#define PITCH_CALIBRATION_PROMPT_PERIOD_MS 5000U /* 操作说明重提示间隔，单位 ms（提醒操作者记录上下边界）。 */
#endif /* TASK_PITCH_CONFIG_H */




