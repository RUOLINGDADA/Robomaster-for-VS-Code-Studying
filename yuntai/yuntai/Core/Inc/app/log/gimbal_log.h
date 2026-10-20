/**
 * @file gimbal_log.h
 * @brief 云台诊断共用的文本格式和本周期快照参数。
 *
 * 调用点用 LOG_TRY_PRINTF(category, GIMBAL_LOG_FORMAT, GIMBAL_LOG_ARGS(...))。
 * 宏展开后直接进入唯一日志后端，不再转发日志函数。只传有效轴指针和只读名称。
 * axis 参数只传任务独占的指针，不传自增或函数调用。日志不重新读取 CAN 反馈。
 */
#ifndef APP_GIMBAL_LOG_H
#define APP_GIMBAL_LOG_H /* 防止云台日志格式重复包含。 */

#include "app/log/log.h"
#include "app/gimbal/gimbal_axis.h"
#include <math.h>

/**
 * @brief 将诊断浮点值缩放为安全的 int32，不修改控制状态。
 * @param value 有单位的诊断值。@param scale 转为整数日志单位的倍率。
 * @retval 缩放后整数；超范围饱和，非有限值显示 0，须结合有效标志读取。
 * @note 仅任务格式化时调用；double 中间值避免 INT32_MAX 转 float 后越界。
 */
static inline int32_t GimbalLog_Scaled(float value, float scale) {
  const double scaled = (double)value * (double)scale;
  if (!isfinite(scaled)) { return 0; }
  if (scaled >= (double)INT32_MAX) { return INT32_MAX; }
  if (scaled <= (double)INT32_MIN) { return INT32_MIN; }
  return (int32_t)scaled;
}

/* 控制角属于标出的反馈参考系；机械编码器单独显示，不能混作惯性姿态误差。 */
#define GIMBAL_LOG_FORMAT \
  "[%s] 反馈=%s 阶段=%s 原因=%s 命令百分之一‰=%ld 人工受限=%u 输入暂停=%u " \
  "目标/实际控制角毫度=%ld/%ld 误差等效count=%ld 编码器count=%ld " \
  "目标/控制反馈速度百分之一rpm=%ld/%ld 编码器rpm=%d " \
  "速度环/补偿/目标/提交/反馈电流=%d/%d/%d/%d/%d 输出许可=%u CAN提交=%u " \
  "机械边界=%s CAN龄ms=%lu 在线=%u 快照=%u IMU快照/有效=%u/%u 陀螺/加计龄ms=%lu/%lu 重置=%lu\r\n"

/* 同一 axis->cycle 快照的格式参数；command_permille 为本周期输入，单位 ‰。 */
#define GIMBAL_LOG_ARGS(axis, label, command_permille) \
  label, GimbalAxis_FeedbackName((axis)->cycle.feedback_source), \
  GimbalAxis_PhaseName((axis)->cycle.phase), GimbalAxis_ReasonName((axis)->cycle.reason), \
  (long)GimbalLog_Scaled((command_permille), 100.0f), \
  (axis)->cycle.control.command_limited ? 1U : 0U, (axis)->cycle.input_inhibited ? 1U : 0U, \
  (long)GimbalLog_Scaled((float)(axis)->cycle.control.target_angle_raw, 360000.0f / GM6020_ENCODER_COUNTS_PER_REV), \
  (long)GimbalLog_Scaled((axis)->cycle.control.feedback_angle_raw, 360000.0f / GM6020_ENCODER_COUNTS_PER_REV), \
  (long)(axis)->cycle.control.angle_error_raw, (long)(axis)->cycle.snapshot.feedback.angle_total_raw, \
  (long)GimbalLog_Scaled((axis)->cycle.control.target_speed_rpm, 100.0f), \
  (long)GimbalLog_Scaled((axis)->cycle.control.feedback_speed_rpm, 100.0f), \
  (axis)->cycle.snapshot.feedback.speed_rpm, (axis)->cycle.control.speed_loop_current_raw, \
  (axis)->cycle.control.gravity_compensation_current_raw, (axis)->cycle.control.target_current_raw, \
  (axis)->cycle.applied_current_raw, (axis)->cycle.snapshot.feedback.current_raw, \
  (axis)->cycle.output_enabled ? 1U : 0U, (axis)->cycle.can_submitted ? 1U : 0U, \
  (axis)->cycle.limit == GM6020_LIMIT_MIN ? "最小" : \
  ((axis)->cycle.limit == GM6020_LIMIT_MAX ? "最大" : "无"), \
  (unsigned long)(axis)->cycle.feedback_age_ms, \
  (axis)->cycle.snapshot_valid && (axis)->cycle.snapshot.online ? 1U : 0U, \
  (axis)->cycle.snapshot_valid ? 1U : 0U, (axis)->cycle.ins_available ? 1U : 0U, \
  (axis)->cycle.ins.attitude_valid ? 1U : 0U, (unsigned long)(axis)->cycle.gyro_age_ms, \
  (unsigned long)(axis)->cycle.accel_age_ms, (unsigned long)(axis)->cycle.ins.reset_sequence

#endif /* APP_GIMBAL_LOG_H */
