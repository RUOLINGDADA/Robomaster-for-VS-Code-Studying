/**
  ******************************************************************************
  * @file    task_yaw_config.h
  * @brief   Yaw 标定数据、硬件调参模式和控制参数入口。
  *
  * 先使用 GM6020 标定测试手动记录中心、最小和最大连续角度，再把记录值
  * 写入本文件并将 YAW_CALIBRATION_VALID 改为 1。Yaw 正式运行时和角度环
  * 硬件测试共同读取这些值，避免“测试一套范围、正式模式另一套范围”。
  * 本文件只保存编译期参数，不访问 CAN、FreeRTOS 或串口。
  ******************************************************************************
  */

#ifndef TASK_YAW_CONFIG_H
#define TASK_YAW_CONFIG_H /* 防止 Yaw 配置被同一编译单元重复包含。 */

#include <stdint.h>

#define YAW_HARDWARE_TEST_MODE_OFF 0U /* 关闭硬件测试，任务运行正式 Yaw 控制。 */
#define YAW_HARDWARE_TEST_MODE_CALIBRATION 1U /* 只读打印角度，禁止主动输出电流。 */
#define YAW_HARDWARE_TEST_MODE_ANGLE_LOOP 2U /* 使用正式闭环保持用户指定角度。 */

#ifndef YAW_HARDWARE_TEST_MODE
#define YAW_HARDWARE_TEST_MODE YAW_HARDWARE_TEST_MODE_CALIBRATION /* 当前烧录模式，默认只读标定。 */
#endif

#define YAW_CALIBRATION_VALID 0U /* 1 表示下面三组角度已完成实机标定。 */
#define YAW_CALIBRATION_CENTER_ANGLE_RAW 0 /* 物理中位连续角度，单位编码器计数。 */
#define YAW_CALIBRATION_MIN_ANGLE_RAW 0 /* 两侧实测值中较小的安全边界，单位计数。 */
#define YAW_CALIBRATION_MAX_ANGLE_RAW 0 /* 两侧实测值中较大的安全边界，单位计数。 */

#define YAW_MAX_COMMAND_SPEED_RPM 80.0f /* 正式遥控相对速度满杆上限，单位 rpm。 */
#define YAW_MAX_SPEED_TARGET_RPM 120.0f /* 角度环生成的速度目标上限，单位 rpm。 */
#define YAW_POSITION_KP_RPM_PER_RAW 0.15f /* 台架起点：角度 P 增益，单位 rpm/计数，大于 0。 */
#define YAW_SPEED_KP_CURRENT_PER_RPM 8.0f /* 台架起点：速度 P 增益，单位原始电流/rpm，非负。 */
#define YAW_SPEED_KI_CURRENT_PER_RPM_S 20.0f /* 速度积分增益，单位原始电流/(rpm·s)。 */
#define YAW_MAX_CURRENT_RAW 3000.0f /* Yaw 闭环电流目标上限，必须不超过 GM6020 16384。 */
#define YAW_CURRENT_SLEW_RAW_PER_S 12000.0f /* 电流变化率上限，单位原始值/秒。 */
#define YAW_SPEED_FILTER_ALPHA 0.20f /* 速度/电流低通本次采样权重，范围 (0, 1]，越小越平滑。 */

#define YAW_STARTUP_GRACE_MS 250U /* 输出刚打开后暂不判断堵转，单位 ms。 */
#define YAW_REVERSAL_GRACE_MS 150U /* 换向后暂不判断堵转，单位 ms。 */
#define YAW_STALL_TIME_MS 300U /* 堵转候选连续成立时间，单位 ms。 */
#define YAW_RELEASE_CONFIRM_MS 100U /* 台架起点：恢复条件连续确认 ms；固定目标须手动反向移开。 */
#define YAW_STALL_SPEED_RPM 3 /* 堵转低速阈值，绝对值单位 rpm。 */
#define YAW_STALL_COMMAND_CURRENT_RAW 900U /* 堵转检测目标电流下限，原始值。 */
#define YAW_STALL_FEEDBACK_CURRENT_RAW 900U /* 堵转检测反馈电流下限，原始值。 */
#define YAW_STALL_POSITION_DELTA_RAW 8U /* 堵转窗口允许的最大角度变化，单位计数。 */
#define YAW_RELEASE_FEEDBACK_CURRENT_RAW 500U /* 解除堵转前允许的反馈电流上限。 */
#define YAW_RELEASE_POSITION_DELTA_RAW 114U /* 台架起点：固定目标反向移开约 5 度，单位计数。 */

#define YAW_ANGLE_LOOP_TARGET_ANGLE_DEG 30.0f /* 角度环测试目标，正负相对中心，单位度。 */
#define YAW_ANGLE_LOOP_LOG_PERIOD_MS 200U /* 角度环测试日志最短间隔，单位 ms。 */

#define YAW_CALIBRATION_LOG_PERIOD_MS 200U /* 标定数据日志间隔，单位 ms。 */
#define YAW_CALIBRATION_PROMPT_PERIOD_MS 5000U /* 操作说明重提示间隔，单位 ms。 */
#endif /* TASK_YAW_CONFIG_H */




