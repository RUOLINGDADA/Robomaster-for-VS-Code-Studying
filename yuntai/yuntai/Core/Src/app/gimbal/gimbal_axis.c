/**
  ******************************************************************************
  * @file    gimbal_axis.c
  * @brief   GM6020 正式角度控制、方向保护和一致诊断的共用运行时。
  *
  * 固定目标只改变输入意图，不更换控制算法。反馈每周期只复制一次；
  * 日志从本周期记录读取，不再拼接下一帧。所有函数由本轴任务独占调用。
  ******************************************************************************
  */
#include "app/gimbal/gimbal_axis.h"
#include "usart.h"
#include <math.h>
#include <stddef.h>

static int16_t GimbalAxis_ToI16(float value) {
  if (value > 32767.0f) { return 32767; }
  if (value < -32768.0f) { return -32768; }
  return (int16_t)value;
}

static uint32_t GimbalAxis_FeedbackAge(uint32_t now_ms, uint32_t stamp_ms) {
  const uint32_t age = now_ms - stamp_ms;
  /* ISR 可在任务取 now_ms 后更新一帧；最多略晚的时间戳按 0 ms 处理。 */
  return age <= INT32_MAX ? age : 0U;
}

static bool GimbalAxis_CheckParameters(const GimbalAxis_ConfigTypeDef *c) {
  return isfinite(c->control.position_kp_rpm_per_raw) &&
         c->control.position_kp_rpm_per_raw > 0.0f &&
         isfinite(c->control.speed_kp_current_per_rpm) &&
         c->control.speed_kp_current_per_rpm >= 0.0f &&
         isfinite(c->control.speed_ki_current_per_rpm_s) &&
         c->control.speed_ki_current_per_rpm_s >= 0.0f &&
         isfinite(c->control.max_command_speed_rpm) &&
         c->control.max_command_speed_rpm > 0.0f &&
         isfinite(c->control.max_speed_target_rpm) &&
         c->control.max_speed_target_rpm > 0.0f &&
         isfinite(c->control.max_current_raw) &&
         c->control.max_current_raw > 0.0f &&
         c->control.max_current_raw <= GM6020_CURRENT_RAW_MAX &&
         isfinite(c->control.current_slew_raw_per_s) &&
         c->control.current_slew_raw_per_s > 0.0f &&
         c->control.speed_filter_alpha > 0.0f &&
         c->control.speed_filter_alpha <= 1.0f &&
         c->protection.stall_time_ms > 0U &&
         c->protection.release_confirm_ms > 0U &&
         c->protection.release_position_delta_raw > 0U &&
         c->protection.stall_speed_threshold_rpm >= 0 &&
         c->protection.stall_command_current_threshold_raw > 0U &&
         c->protection.stall_feedback_current_threshold_raw > 0U;
}

bool GimbalAxis_Init(GimbalAxis_HandleTypeDef *axis,
                     const GimbalAxis_ConfigTypeDef *config) {
  if (axis == NULL || config == NULL) {
    return false;
  }
  *axis = (GimbalAxis_HandleTypeDef){0};
  axis->config = *config;
  if (axis->config.motor.feedback_timeout_ms == 0U) {
    axis->config.motor.feedback_timeout_ms = GM6020_FEEDBACK_TIMEOUT_MS;
  }
  const Gm6020_CalibrationTypeDef *cal = &axis->config.calibration;
  axis->calibration_valid = cal->valid &&
                             cal->min_angle_raw < cal->center_angle_raw &&
                             cal->center_angle_raw < cal->max_angle_raw;
  axis->parameters_valid = GimbalAxis_CheckParameters(config);
  /* 范围和首帧参考只由同一份标定生成，不能在测试、正式配置里各存一套。 */
  axis->config.motor.mechanical_min_raw = axis->calibration_valid
                                            ? cal->min_angle_raw
                                            : GM6020_ANGLE_LIMIT_DISABLED_MIN;
  axis->config.motor.mechanical_max_raw = axis->calibration_valid
                                            ? cal->max_angle_raw
                                            : GM6020_ANGLE_LIMIT_DISABLED_MAX;
  axis->config.motor.angle_reference_enabled = axis->calibration_valid;
  const int32_t counts = (int32_t)GM6020_ENCODER_COUNTS_PER_REV;
  axis->config.motor.angle_reference_single_raw =
      (uint16_t)((cal->center_angle_raw % counts + counts) % counts);
  axis->config.motor.angle_reference_total_raw = cal->center_angle_raw;
  axis->config.control.center_angle_raw = cal->center_angle_raw;
  axis->config.control.min_angle_raw = cal->min_angle_raw;
  axis->config.control.max_angle_raw = cal->max_angle_raw;
  axis->config.protection.min_angle_raw = cal->min_angle_raw;
  axis->config.protection.max_angle_raw = cal->max_angle_raw;
  if (!Gm6020_Init(&axis->motor, &axis->config.motor)) {
    return false;
  }
  GimbalProtection_Init(&axis->protection, &axis->config.protection);
  axis->initialized = true;
  axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
  return true;
}

bool GimbalAxis_SendZero(GimbalAxis_HandleTypeDef *axis) {
  if (axis == NULL || !axis->initialized) {
    return false;
  }
  axis->cycle.applied_current_raw = 0;
  axis->cycle.output_enabled = false;
  (void)Gm6020_SetOutputEnabled(&axis->motor, false);
  (void)Gm6020_SetCurrent(&axis->motor, 0);
  axis->cycle.can_submitted = Gm6020_Send(&axis->motor);
  return axis->cycle.can_submitted;
}

const char *GimbalAxis_PhaseName(GimbalAxis_PhaseTypeDef phase) {
  switch (phase) {
  case GIMBAL_AXIS_PHASE_WAIT_FEEDBACK: return "等待反馈";
  case GIMBAL_AXIS_PHASE_HOLD_POSITION: return "保持位置";
  case GIMBAL_AXIS_PHASE_ACTIVE: return "主动控制";
  case GIMBAL_AXIS_PHASE_LIMIT_HOLD: return "软件限位保持";
  case GIMBAL_AXIS_PHASE_STALL_HOLD: return "堵转保持";
  case GIMBAL_AXIS_PHASE_FEEDBACK_LOST: return "反馈丢失";
  default: return "初始化";
  }
}

static void GimbalAxis_ResetController(GimbalAxis_HandleTypeDef *axis) {
  GimbalControl_ResetToAngle(&axis->controller,
                             axis->cycle.snapshot.feedback.angle_total_raw);
  LowPassFilter_Reset(&axis->current_filter,
                       (float)axis->cycle.snapshot.feedback.current_raw);
}

/*
 * 周期顺序：反馈与配置检查 → 首帧/掉线安全周期 → 生成意图 → 控制计算
 * → 保护 → 电流提交。所有提前退出同样记录阶段和零命令，日志可以诊断失败。
 */
static void GimbalAxis_Run(GimbalAxis_HandleTypeDef *axis,
                            const Gimbal_CommandTypeDef *command,
                            bool fixed_target, int32_t target_angle_raw,
                            uint32_t now_ms, uint32_t dt_ms) {
  if (axis == NULL || !axis->initialized) {
    return;
  }
  axis->cycle = (GimbalAxis_OutputTypeDef){0};
  axis->cycle.control.target_angle_raw = fixed_target
      ? target_angle_raw : (int32_t)axis->controller.target_angle_raw;
  (void)Gm6020_Process(&axis->motor, now_ms);
  axis->cycle.snapshot_valid =
      Gm6020_GetSnapshot(&axis->motor, &axis->cycle.snapshot);
  axis->cycle.limit = axis->cycle.snapshot.limit;
  if (fixed_target && axis->cycle.snapshot_valid) {
    const int64_t error_raw = (int64_t)target_angle_raw -
                              axis->cycle.snapshot.feedback.angle_total_raw;
    axis->cycle.control.angle_error_raw =
        error_raw > INT32_MAX ? INT32_MAX :
        (error_raw < INT32_MIN ? INT32_MIN : (int32_t)error_raw);
  }
  axis->cycle.feedback_age_ms = GimbalAxis_FeedbackAge(
      now_ms, axis->cycle.snapshot.feedback.last_feedback_tick);

  if (!axis->cycle.snapshot_valid || !axis->cycle.snapshot.online ||
      axis->cycle.feedback_age_ms >= axis->config.motor.feedback_timeout_ms) {
    GimbalProtection_OnFeedbackLost(&axis->protection);
    axis->feedback_lost = axis->controller_initialized;
    axis->cycle.phase = axis->controller_initialized
                            ? GIMBAL_AXIS_PHASE_FEEDBACK_LOST
                            : GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_PROTECTION_FEEDBACK_LOST;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (!axis->calibration_valid || !axis->parameters_valid || dt_ms == 0U ||
      command == NULL || command->velocity_permille < -1000 ||
      command->velocity_permille > 1000) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = !axis->calibration_valid
                             ? GIMBAL_PROTECTION_CALIBRATION_INVALID
                             : GIMBAL_PROTECTION_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (fixed_target &&
      (target_angle_raw < axis->config.calibration.min_angle_raw ||
       target_angle_raw > axis->config.calibration.max_angle_raw)) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_LIMIT_HOLD;
    axis->cycle.reason = GIMBAL_PROTECTION_TARGET_INVALID;
    if (axis->controller_initialized) {
      GimbalAxis_ResetController(axis);
    }
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (!axis->controller_initialized) {
    GimbalControl_Init(&axis->controller, &axis->config.control,
                        axis->cycle.snapshot.feedback.angle_total_raw);
    LowPassFilter_Init(&axis->current_filter,
                        (float)axis->cycle.snapshot.feedback.current_raw,
                        axis->config.control.speed_filter_alpha);
    axis->controller_initialized = axis->controller.initialized;
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis); /* 首次首帧先发零，下周期才开始闭环。 */
    return;
  }
  if (axis->feedback_lost) {
    axis->feedback_lost = false;
    GimbalAxis_ResetController(axis);
    axis->cycle.phase = axis->protection.reason == GIMBAL_PROTECTION_STALL_HOLD
                            ? GIMBAL_AXIS_PHASE_STALL_HOLD
                            : GIMBAL_AXIS_PHASE_HOLD_POSITION;
    axis->cycle.reason = axis->protection.reason;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (fixed_target) {
    (void)GimbalControl_SetTargetAngle(&axis->controller, target_angle_raw);
  }
  const int64_t error = (int64_t)target_angle_raw -
                       axis->cycle.snapshot.feedback.angle_total_raw;
  const int16_t intent = fixed_target ? (error > 0 ? 1 : (error < 0 ? -1 : 0))
                                     : (command->enabled
                                            ? command->velocity_permille : 0);
  /* 清除硬限位只是解除锁，输出仍是关闭状态；只有明确向内意图可解除。 */
  if ((axis->cycle.limit == GM6020_LIMIT_MIN && intent > 0) ||
      (axis->cycle.limit == GM6020_LIMIT_MAX && intent < 0)) {
    if (Gm6020_ClearAngleLimit(&axis->motor)) {
      axis->cycle.limit = GM6020_LIMIT_NONE;
    }
  }

  const bool stall_hold =
      axis->protection.reason == GIMBAL_PROTECTION_STALL_HOLD;
  GimbalControl_Update(&axis->controller, &axis->cycle.snapshot.feedback,
                        command, (float)dt_ms / 1000.0f, stall_hold,
                        &axis->cycle.control);
  /*
   * 保护层必须判断“本周期是否确实请求了驱动力”。这里不能继续使用
   * snapshot.output_enabled：那是取快照时的旧状态，若拿它判断启动或换向，
   * 宽限和堵转候选会整体滞后一周期，日志也会把旧许可误报成当前许可。
   */
  const bool output_requested = axis->cycle.control.target_current_raw != 0;
  const GimbalProtection_InputTypeDef input = {
      .feedback_online = true,
      .output_enabled = output_requested,
      .fixed_target = fixed_target,
      .angle_total_raw = axis->cycle.snapshot.feedback.angle_total_raw,
      .filtered_speed_rpm = GimbalAxis_ToI16(axis->controller.speed_filter.value),
      .filtered_current_raw = GimbalAxis_ToI16(LowPassFilter_Update(
          &axis->current_filter,
          (float)axis->cycle.snapshot.feedback.current_raw)),
      .requested_current_raw = axis->cycle.control.target_current_raw,
      .command_velocity_permille = intent,
      .now_ms = now_ms,
  };
  GimbalProtection_OutputTypeDef protection = {0};
  GimbalProtection_Update(&axis->protection, &input, &protection);
  axis->cycle.reason = protection.reason;
  axis->cycle.blocked_direction = protection.blocked_direction;
  axis->cycle.candidate_ms = axis->protection.candidate_active
      ? now_ms - axis->protection.candidate_start_ms : 0U;
  if (protection.entered || protection.cleared) {
    GimbalAxis_ResetController(axis); /* 立即清积分和 Ramp，禁止旧力矩追赶。 */
  }
  if (protection.allow_output && axis->cycle.limit == GM6020_LIMIT_NONE) {
    /*
     * 先打开许可再设置电流；设置函数再次执行协议硬限位检查。
     * 若本周期快照之后 ISR 已触发硬限位，SetCurrent 会返回拒绝，仍走零输出。
     */
    const bool enabled = Gm6020_SetOutputEnabled(&axis->motor, true);
    const bool current_set =
        enabled && Gm6020_SetCurrent(&axis->motor,
                                     axis->cycle.control.target_current_raw);
    if (enabled && current_set) {
      axis->cycle.output_enabled = true;
      axis->cycle.applied_current_raw = axis->cycle.control.target_current_raw;
      axis->cycle.can_submitted = Gm6020_Send(&axis->motor);
    } else {
      (void)GimbalAxis_SendZero(axis);
    }
  } else {
    (void)GimbalAxis_SendZero(axis);
  }
  axis->cycle.phase = axis->cycle.reason == GIMBAL_PROTECTION_STALL_HOLD
      ? GIMBAL_AXIS_PHASE_STALL_HOLD
      : ((axis->cycle.reason == GIMBAL_PROTECTION_SOFT_MIN ||
          axis->cycle.reason == GIMBAL_PROTECTION_SOFT_MAX ||
          axis->cycle.limit != GM6020_LIMIT_NONE)
             ? GIMBAL_AXIS_PHASE_LIMIT_HOLD
             : ((fixed_target ? (error > 1 || error < -1) :
                              (command->enabled && command->velocity_permille != 0))
                    ? GIMBAL_AXIS_PHASE_ACTIVE
                    : GIMBAL_AXIS_PHASE_HOLD_POSITION));
  if (axis->cycle.limit != GM6020_LIMIT_NONE &&
      axis->cycle.reason == GIMBAL_PROTECTION_NONE) {
    axis->cycle.reason = axis->cycle.limit == GM6020_LIMIT_MIN
                            ? GIMBAL_PROTECTION_SOFT_MIN
                            : GIMBAL_PROTECTION_SOFT_MAX;
  }
  if (protection.entered || protection.cleared) {
    axis->event = axis->cycle;
    axis->event_name = protection.entered ? "堵转触发" : "保护解除";
  }
}

void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms) {
  GimbalAxis_Run(axis, command, false, 0, now_ms, dt_ms);
}

void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms) {
  const Gimbal_CommandTypeDef command = {0};
  GimbalAxis_Run(axis, &command, true, target_angle_raw, now_ms, dt_ms);
}

bool GimbalAxis_TryLog(GimbalAxis_HandleTypeDef *axis,
                       const char *label, int16_t command_permille) {
  if (axis == NULL || label == NULL) {
    return false;
  }
  const GimbalAxis_OutputTypeDef *r =
      axis->event_name != NULL ? &axis->event : &axis->cycle;
  const int64_t offset_cdeg =
      ((int64_t)r->control.target_angle_raw -
       axis->config.calibration.center_angle_raw) * 36000LL /
      GM6020_ENCODER_COUNTS_PER_REV;
  const uint64_t offset_abs = offset_cdeg < 0 ? -offset_cdeg : offset_cdeg;
  /*
   * 使用整数输出小数角度，兼容 newlib-nano 默认没有 printf 浮点支持的配置。
   * applied 是保护后的命令；requested 是触发前计算值；提交只说明 HAL 接受。
   */
  const bool accepted = usart_try_printf(
      "[%s] 事件=%s 阶段=%s 原因=%s 命令=%d 目标偏角=%s%lu.%02lu度 "
      "目标角度=%ld 实际角度=%ld 误差(计数)=%ld 目标速度(百分之一rpm)=%ld "
      "实际速度(rpm)=%d 计算电流=%d 目标电流=%d 反馈电流=%d 输出许可=%u "
      "CAN提交=%u 限位=%s 堵转方向=%d 候选(ms)=%lu 反馈年龄(ms)=%lu 在线=%u 快照有效=%u "
      "温度(度C)=%u 保留字节=0x%02X\r\n",
      label, axis->event_name != NULL ? axis->event_name : "周期诊断",
      GimbalAxis_PhaseName(r->phase), GimbalProtection_ReasonName(r->reason),
      command_permille, offset_cdeg < 0 ? "-" : "",
      (unsigned long)(offset_abs / 100U), (unsigned long)(offset_abs % 100U),
      (long)r->control.target_angle_raw, (long)r->snapshot.feedback.angle_total_raw,
      (long)r->control.angle_error_raw, (long)(r->control.target_speed_rpm * 100.0f),
      r->snapshot.feedback.speed_rpm, r->control.target_current_raw,
      r->applied_current_raw, r->snapshot.feedback.current_raw,
      r->output_enabled ? 1U : 0U, r->can_submitted ? 1U : 0U,
      r->limit == GM6020_LIMIT_MIN ? "最小" :
          (r->limit == GM6020_LIMIT_MAX ? "最大" : "无"),
      r->blocked_direction, (unsigned long)r->candidate_ms, (unsigned long)r->feedback_age_ms,
      r->snapshot_valid && r->snapshot.online ? 1U : 0U,
      r->snapshot_valid ? 1U : 0U, r->snapshot.feedback.temperature_c,
      r->snapshot.feedback.reserved_raw);
  if (accepted) {
    axis->event_name = NULL;
  }
  return accepted;
}
