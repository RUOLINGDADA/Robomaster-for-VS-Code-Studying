/**
  ******************************************************************************
  * @file    task_pitch_config.h
  * @brief   Pitch 轴 GM6020 标定、角度环测试和正式模式配置。
  *
  * 参数独立于 Yaw；先用标定模式记录本轴中心及上下机械边界，再切换到
  * 角度环模式。未完成标定时任何模式都保持零输出。
  ******************************************************************************
  */

#ifndef TASK_PITCH_CONFIG_H
#define TASK_PITCH_CONFIG_H /* 防止 Pitch 配置重复包含。 */

#define PITCH_HARDWARE_TEST_MODE_OFF 0U /* 关闭硬件测试，当前正式路径保持零输出。 */
#define PITCH_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读标定，强制零电流。 */
#define PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 复用正式闭环保持固定角度。 */
#ifndef PITCH_HARDWARE_TEST_MODE
#define PITCH_HARDWARE_TEST_MODE PITCH_HARDWARE_TEST_MODE_CALIBRATION /* 当前烧录模式，默认只读标定。 */
#endif

#define PITCH_CALIBRATION_VALID 0U /* 1 表示本轴三组角度已实机确认。 */
#define PITCH_CALIBRATION_CENTER_ANGLE_RAW 0 /* 中位连续角度，单位编码器计数。 */
#define PITCH_CALIBRATION_MIN_ANGLE_RAW 0 /* 较小角度边界，单位编码器计数。 */
#define PITCH_CALIBRATION_MAX_ANGLE_RAW 0 /* 较大角度边界，单位编码器计数。 */

#define PITCH_MAX_COMMAND_SPEED_RPM 50.0f /* Pitch 正式输入满量程速度上限，单位 rpm。 */
#define PITCH_MAX_SPEED_TARGET_RPM 80.0f /* Pitch 角度环速度目标上限，单位 rpm。 */
#define PITCH_POSITION_KP_RPM_PER_RAW 0.15f /* 台架起点：角度 P 增益，单位 rpm/计数，大于 0。 */
#define PITCH_SPEED_KP_CURRENT_PER_RPM 8.0f /* 台架起点：速度 P 增益，单位原始电流/rpm，非负。 */
#define PITCH_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度积分增益，单位原始值/(rpm·s)。 */
#define PITCH_MAX_CURRENT_RAW 2500.0f /* Pitch 电流上限，必须不超过 GM6020 16384。 */
#define PITCH_CURRENT_SLEW_RAW_PER_S 10000.0f /* 电流变化率上限，单位原始值/秒。 */
#define PITCH_SPEED_FILTER_ALPHA 0.20f /* 速度/电流低通本次采样权重，范围 (0, 1]，越小越平滑。 */

#define PITCH_STARTUP_GRACE_MS 250U /* 启动后不判断堵转的宽限，单位 ms。 */
#define PITCH_REVERSAL_GRACE_MS 150U /* 换向后不判断堵转的宽限，单位 ms。 */
#define PITCH_STALL_TIME_MS 300U /* 堵转证据连续成立时间，单位 ms。 */
#define PITCH_RELEASE_CONFIRM_MS 100U /* 台架起点：手动反向移开后连续确认时间，单位 ms。 */
#define PITCH_STALL_SPEED_RPM 3 /* 低速堵转阈值，绝对值单位 rpm。 */
#define PITCH_STALL_COMMAND_CURRENT_RAW 700U /* 堵转目标电流下限，原始值。 */
#define PITCH_STALL_FEEDBACK_CURRENT_RAW 700U /* 堵转反馈电流下限，原始值。 */
#define PITCH_STALL_POSITION_DELTA_RAW 8U /* 堵转检测窗口位移上限，单位计数。 */
#define PITCH_RELEASE_FEEDBACK_CURRENT_RAW 400U /* 解除前允许的反馈电流上限。 */
#define PITCH_RELEASE_POSITION_DELTA_RAW 114U /* 台架起点：手动反向移开约 5 度，单位计数。 */

#define PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG 0.0f /* 固定目标相对中位角度，单位度，正负和小数均可，须在边界内。 */
#define PITCH_ANGLE_LOOP_LOG_PERIOD_MS 200U /* Pitch 角度测试日志间隔，单位 ms。 */

#define PITCH_CALIBRATION_LOG_PERIOD_MS 200U /* 标定数据日志间隔，单位 ms。 */
#define PITCH_CALIBRATION_PROMPT_PERIOD_MS 5000U /* 操作说明重提示间隔，单位 ms。 */
#endif /* TASK_PITCH_CONFIG_H */



