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

/* 目标相对标定中位的角度，百分之一度；64 位差值避免连续角度相减溢出。 */
#define GIMBAL_LOG_OFFSET_CDEG(axis) \
  (((int64_t)(axis)->cycle.control.target_angle_raw - (axis)->config.calibration.center_angle_raw) * \
   36000LL / GM6020_ENCODER_COUNTS_PER_REV)
/* 偏角绝对值，百分之一度；仅用于输出整数和两位小数。 */
#define GIMBAL_LOG_OFFSET_ABS_CDEG(axis) \
  ((uint64_t)(GIMBAL_LOG_OFFSET_CDEG(axis) < 0 ? -GIMBAL_LOG_OFFSET_CDEG(axis) : GIMBAL_LOG_OFFSET_CDEG(axis)))

/* 正式控制和固定目标测试共用字段；电流为协议原始值，角度为连续 count。 */
#define GIMBAL_LOG_FORMAT \
  "[%s] 阶段=%s 原因=%s 命令(百分之一‰)=%ld 目标偏角=%s%lu.%02lu度 目标角度=%ld 实际角度=%ld " \
  "误差=%ld 目标速度(百分之一rpm)=%ld 实际速度=%d 速度环电流=%d 重力补偿=%d " \
  "目标电流=%d 实际发送电流=%d 反馈电流=%d 输出许可=%u CAN提交=%u 限位=%s " \
  "反馈年龄(ms)=%lu 在线=%u 快照有效=%u 温度=%u 保留字节=0x%02X\r\n"

/* 同一 axis->cycle 快照的格式参数；command_permille 为本周期输入，单位 ‰。 */
#define GIMBAL_LOG_ARGS(axis, label, command_permille) \
  label, GimbalAxis_PhaseName((axis)->cycle.phase), GimbalAxis_ReasonName((axis)->cycle.reason), \
  (long)((command_permille) * 100.0f), GIMBAL_LOG_OFFSET_CDEG(axis) < 0 ? "-" : "", \
  (unsigned long)(GIMBAL_LOG_OFFSET_ABS_CDEG(axis) / 100U), (unsigned long)(GIMBAL_LOG_OFFSET_ABS_CDEG(axis) % 100U), \
  (long)(axis)->cycle.control.target_angle_raw, (long)(axis)->cycle.snapshot.feedback.angle_total_raw, \
  (long)(axis)->cycle.control.angle_error_raw, (long)((axis)->cycle.control.target_speed_rpm * 100.0f), \
  (axis)->cycle.snapshot.feedback.speed_rpm, (axis)->cycle.control.speed_loop_current_raw, \
  (axis)->cycle.control.gravity_compensation_current_raw, (axis)->cycle.control.target_current_raw, \
  (axis)->cycle.applied_current_raw, (axis)->cycle.snapshot.feedback.current_raw, \
  (axis)->cycle.output_enabled ? 1U : 0U, (axis)->cycle.can_submitted ? 1U : 0U, \
  (axis)->cycle.limit == GM6020_LIMIT_MIN ? "最小" : \
  ((axis)->cycle.limit == GM6020_LIMIT_MAX ? "最大" : "无"), \
  (unsigned long)(axis)->cycle.feedback_age_ms, \
  (axis)->cycle.snapshot_valid && (axis)->cycle.snapshot.online ? 1U : 0U, \
  (axis)->cycle.snapshot_valid ? 1U : 0U, (axis)->cycle.snapshot.feedback.temperature_c, \
  (axis)->cycle.snapshot.feedback.reserved_raw

#endif /* APP_GIMBAL_LOG_H */
