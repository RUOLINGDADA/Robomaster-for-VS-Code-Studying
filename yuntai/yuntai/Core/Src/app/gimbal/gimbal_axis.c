/**
  ******************************************************************************
  * @file    gimbal_axis.c
  * @brief   GM6020 角度闭环、软件边界过滤和一致诊断。
  *
  * 控制周期只复制一次反馈快照，随后按“反馈检查→目标处理→角度/速度环
  * →电流提交”的顺序执行。软件边界不再切换固定电流或堵转锁存；它只
  * 阻止继续向外的输入，并让现有闭环保持当前反馈位置。
  ******************************************************************************
  */
#include "app/gimbal/gimbal_axis.h"
#include "usart.h"
#include <math.h>
#include <stddef.h>

/* 将浮点速度安全转换为有符号 16 位，避免 -32768 取反或越界转换。 */
static int16_t GimbalAxis_ToI16(float value) {
  if (value > 32767.0f) {
    return 32767;
  }
  if (value < -32768.0f) {
    return -32768;
  }
  return (int16_t)value;
}

/* 将电调反馈转速映射到连续角度逻辑方向。 */
static int16_t GimbalAxis_ToLogicalSpeed(int16_t physical_speed_rpm,
                                         int8_t speed_sign) {
  return GimbalAxis_ToI16((float)((int32_t)physical_speed_rpm * speed_sign));
}

/* 处理 HAL 毫秒回绕和任务取时间与 ISR 更新时间的竞态。 */
static uint32_t GimbalAxis_FeedbackAge(uint32_t now_ms,
                                       uint32_t stamp_ms) {
  const uint32_t age_ms = now_ms - stamp_ms;
  return age_ms <= INT32_MAX ? age_ms : 0U;
}

/* 参数无效时禁止闭环，避免 NaN、越界增益或错误滤波权重进入电流计算。 */
static bool GimbalAxis_CheckParameters(const GimbalAxis_ConfigTypeDef *config) {
  const GimbalControl_ConfigTypeDef *c = &config->control;
  return isfinite(c->position_kp_rpm_per_raw) &&
         c->position_kp_rpm_per_raw > 0.0f &&
         isfinite(c->position_ki_rpm_per_raw_s) &&
         c->position_ki_rpm_per_raw_s >= 0.0f &&
         isfinite(c->position_integral_limit_rpm) &&
         c->position_integral_limit_rpm >= 0.0f &&
         isfinite(c->position_kd_rpm_s_per_raw) &&
         c->position_kd_rpm_s_per_raw >= 0.0f &&
         isfinite(c->speed_kp_current_per_rpm) &&
         c->speed_kp_current_per_rpm >= 0.0f &&
         isfinite(c->speed_ki_current_per_rpm_s) &&
         c->speed_ki_current_per_rpm_s >= 0.0f &&
         isfinite(c->speed_kd_current_s_per_rpm) &&
         c->speed_kd_current_s_per_rpm >= 0.0f &&
         isfinite(c->speed_integral_limit_raw) &&
         c->speed_integral_limit_raw >= 0.0f &&
         isfinite(c->velocity_feedforward_gain) &&
         c->velocity_feedforward_gain >= 0.0f &&
         isfinite(c->max_command_speed_rpm) &&
         c->max_command_speed_rpm > 0.0f &&
         isfinite(c->max_speed_target_rpm) &&
         c->max_speed_target_rpm > 0.0f &&
         isfinite(c->max_current_raw) &&
         c->max_current_raw > 0.0f &&
         c->max_current_raw <= GM6020_CURRENT_RAW_MAX &&
         isfinite(c->current_slew_raw_per_s) &&
         c->current_slew_raw_per_s > 0.0f &&
         c->speed_filter_alpha > 0.0f && c->speed_filter_alpha <= 1.0f &&
         isfinite(c->gravity_compensation_bias_current_raw) &&
         isfinite(c->gravity_compensation_amplitude_current_raw) &&
         c->gravity_compensation_bias_current_raw >= 0.0f &&
         c->gravity_compensation_amplitude_current_raw >= 0.0f &&
         c->gravity_compensation_bias_current_raw +
             c->gravity_compensation_amplitude_current_raw <= c->max_current_raw &&
         c->speed_integral_limit_raw <= c->max_current_raw &&
         c->position_integral_limit_rpm <= c->max_speed_target_rpm;
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
  const Gm6020_CalibrationTypeDef *calibration = &axis->config.calibration;
  axis->calibration_valid = calibration->valid &&
                             calibration->min_angle_raw < calibration->center_angle_raw &&
                             calibration->center_angle_raw < calibration->max_angle_raw;
  axis->parameters_valid = GimbalAxis_CheckParameters(config);
  axis->config.motor.mechanical_min_raw = axis->calibration_valid
      ? calibration->min_angle_raw : GM6020_ANGLE_LIMIT_DISABLED_MIN;
  axis->config.motor.mechanical_max_raw = axis->calibration_valid
      ? calibration->max_angle_raw : GM6020_ANGLE_LIMIT_DISABLED_MAX;
  axis->config.motor.angle_reference_enabled = axis->calibration_valid;
  const int32_t counts = (int32_t)GM6020_ENCODER_COUNTS_PER_REV;
  axis->config.motor.angle_reference_single_raw =
      (uint16_t)((calibration->center_angle_raw % counts + counts) % counts);
  axis->config.motor.angle_reference_total_raw = calibration->center_angle_raw;
  axis->config.control.center_angle_raw = calibration->center_angle_raw;
  axis->config.control.min_angle_raw = calibration->min_angle_raw;
  axis->config.control.max_angle_raw = calibration->max_angle_raw;
  if (!Gm6020_Init(&axis->motor, &axis->config.motor)) {
    return false;
  }
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
  case GIMBAL_AXIS_PHASE_LIMIT_HOLD: return "软件限位闭环保持";
  case GIMBAL_AXIS_PHASE_FEEDBACK_LOST: return "反馈丢失";
  default: return "初始化";
  }
}

const char *GimbalAxis_ReasonName(GimbalAxis_ReasonTypeDef reason) {
  switch (reason) {
  case GIMBAL_AXIS_REASON_SOFT_MIN: return "软件最小限位";
  case GIMBAL_AXIS_REASON_SOFT_MAX: return "软件最大限位";
  case GIMBAL_AXIS_REASON_FEEDBACK_LOST: return "反馈丢失";
  case GIMBAL_AXIS_REASON_CALIBRATION_INVALID: return "未完成角度标定";
  case GIMBAL_AXIS_REASON_TARGET_INVALID: return "目标无效或越界";
  case GIMBAL_AXIS_REASON_CONTROL_INVALID: return "控制参数无效";
  case GIMBAL_AXIS_REASON_INPUT_HOLD: return "遥控输入保持";
  default: return "无";
  }
}

/* 清理旧积分和 Ramp，避免反馈恢复后突然追逐掉线前的目标。 */
static void GimbalAxis_ResetController(GimbalAxis_HandleTypeDef *axis) {
  GimbalControl_ResetToAngle(&axis->controller,
                             axis->cycle.snapshot.feedback.angle_total_raw);
}

/* 统一执行一周期控制；固定目标和遥控目标只在调用前提供不同目标输入。 */
static void GimbalAxis_Run(GimbalAxis_HandleTypeDef *axis,
                           const Gimbal_CommandTypeDef *command,
                           bool fixed_target, int32_t target_angle_raw,
                           uint32_t now_ms, uint32_t dt_ms) {
  if (axis == NULL || !axis->initialized) {
    return;
  }
  axis->cycle = (GimbalAxis_OutputTypeDef){0};
  (void)Gm6020_Process(&axis->motor, now_ms);
  axis->cycle.snapshot_valid =
      Gm6020_GetSnapshot(&axis->motor, &axis->cycle.snapshot);
  if (axis->cycle.snapshot_valid) {
    axis->cycle.snapshot.feedback.speed_rpm = GimbalAxis_ToLogicalSpeed(
        axis->cycle.snapshot.feedback.speed_rpm, axis->config.motor.speed_sign);
    axis->cycle.limit = axis->cycle.snapshot.limit;
    axis->cycle.feedback_age_ms = GimbalAxis_FeedbackAge(
        now_ms, axis->cycle.snapshot.feedback.last_feedback_tick);
  }

  if (!axis->cycle.snapshot_valid || !axis->cycle.snapshot.online ||
      axis->cycle.feedback_age_ms >= axis->config.motor.feedback_timeout_ms) {
    axis->feedback_lost = axis->controller_initialized;
    axis->cycle.phase = axis->controller_initialized
        ? GIMBAL_AXIS_PHASE_FEEDBACK_LOST : GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_FEEDBACK_LOST;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (!axis->calibration_valid || !axis->parameters_valid || dt_ms == 0U) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = !axis->calibration_valid
        ? GIMBAL_AXIS_REASON_CALIBRATION_INVALID
        : GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (command == NULL || command->velocity_permille < -1000 ||
      command->velocity_permille > 1000) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (fixed_target && (target_angle_raw < axis->config.calibration.min_angle_raw ||
                       target_angle_raw > axis->config.calibration.max_angle_raw)) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_WAIT_FEEDBACK;
    axis->cycle.reason = GIMBAL_AXIS_REASON_TARGET_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (!axis->controller_initialized) {
    GimbalControl_Init(&axis->controller, &axis->config.control,
                       axis->cycle.snapshot.feedback.angle_total_raw);
    axis->controller_initialized = axis->controller.initialized;
    if (!axis->controller_initialized) {
      axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
      (void)GimbalAxis_SendZero(axis);
      return;
    }
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (axis->feedback_lost) {
    axis->feedback_lost = false;
    GimbalAxis_ResetController(axis);
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
    (void)GimbalAxis_SendZero(axis);
    return;
  }

  if (fixed_target) {
    (void)GimbalControl_SetTargetAngle(&axis->controller, target_angle_raw);
  }
  GimbalControl_Update(&axis->controller, &axis->cycle.snapshot.feedback,
                       command, (float)dt_ms / 1000.0f, &axis->cycle.control);
  if (axis->cycle.limit == GM6020_LIMIT_MIN || axis->cycle.control.at_min_limit) {
    axis->cycle.limit = GM6020_LIMIT_MIN;
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MIN;
  } else if (axis->cycle.limit == GM6020_LIMIT_MAX || axis->cycle.control.at_max_limit) {
    axis->cycle.limit = GM6020_LIMIT_MAX;
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MAX;
  } else if (!command->enabled || command->velocity_permille == 0) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_INPUT_HOLD;
  }

  const int32_t physical_current =
      (int32_t)axis->cycle.control.target_current_raw * axis->config.motor.current_sign;
  const int16_t physical_current_i16 = physical_current > INT16_MAX ? INT16_MAX :
      (physical_current < INT16_MIN ? INT16_MIN : (int16_t)physical_current);
  const bool current_set = Gm6020_SetCurrent(&axis->motor, physical_current_i16);
  const bool enabled = current_set && Gm6020_SetOutputEnabled(&axis->motor, true);
  if (enabled) {
    axis->cycle.output_enabled = true;
    axis->cycle.applied_current_raw = physical_current_i16;
    axis->cycle.can_submitted = Gm6020_Send(&axis->motor);
  } else {
    (void)GimbalAxis_SendZero(axis);
  }

  if (axis->cycle.limit != GM6020_LIMIT_NONE) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_LIMIT_HOLD;
  } else if (command->velocity_permille != 0) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_ACTIVE;
  } else {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
  }
}

void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms) {
  /* 命令失效、超时或数值越界都等价于零速度保持；只有反馈失效才走零输出路径。 */
  const Gimbal_CommandTypeDef hold_command = {
      .velocity_permille = 0, .enabled = true, .fixed_target = false,
      .timestamp_ms = now_ms};
  const bool command_valid = command != NULL && command->enabled &&
                             command->velocity_permille >= -1000 &&
                             command->velocity_permille <= 1000;
  GimbalAxis_Run(axis, command_valid ? command : &hold_command,
                 false, 0, now_ms, dt_ms);
}

void GimbalAxis_RunDisabledCycle(GimbalAxis_HandleTypeDef *axis, uint32_t now_ms) {
  if (axis == NULL || !axis->initialized) {
    return;
  }
  axis->cycle = (GimbalAxis_OutputTypeDef){0};
  (void)Gm6020_Process(&axis->motor, now_ms);
  axis->cycle.snapshot_valid = Gm6020_GetSnapshot(&axis->motor, &axis->cycle.snapshot);
  axis->cycle.phase = GIMBAL_AXIS_PHASE_FEEDBACK_LOST;
  axis->cycle.reason = GIMBAL_AXIS_REASON_FEEDBACK_LOST;
  axis->controller_initialized = false;
  (void)GimbalAxis_SendZero(axis);
}

void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms) {
  const Gimbal_CommandTypeDef fixed_command = {
      .velocity_permille = 0, .enabled = true, .fixed_target = true,
      .timestamp_ms = now_ms};
  GimbalAxis_Run(axis, &fixed_command, true, target_angle_raw, now_ms, dt_ms);
}

bool GimbalAxis_TryLog(GimbalAxis_HandleTypeDef *axis,
                       const char *label, int16_t command_permille) {
  if (axis == NULL || label == NULL) {
    return false;
  }
  const GimbalAxis_OutputTypeDef *r = &axis->cycle;
  const int64_t offset_cdeg =
      ((int64_t)r->control.target_angle_raw - axis->config.calibration.center_angle_raw) *
      36000LL / GM6020_ENCODER_COUNTS_PER_REV;
  const uint64_t offset_abs = offset_cdeg < 0 ? (uint64_t)-offset_cdeg : (uint64_t)offset_cdeg;
  const bool accepted = usart_try_printf(
      "[%s] 阶段=%s 原因=%s 命令=%d 目标偏角=%s%lu.%02lu度 目标角度=%ld 实际角度=%ld "
      "误差=%ld 目标速度(百分之一rpm)=%ld 实际速度=%d 速度环电流=%d 重力补偿=%d "
      "目标电流=%d 实际发送电流=%d 反馈电流=%d 输出许可=%u CAN提交=%u 限位=%s "
      "反馈年龄(ms)=%lu 在线=%u 快照有效=%u 温度=%u 保留字节=0x%02X\r\n",
      label, GimbalAxis_PhaseName(r->phase), GimbalAxis_ReasonName(r->reason),
      command_permille, offset_cdeg < 0 ? "-" : "",
      (unsigned long)(offset_abs / 100U), (unsigned long)(offset_abs % 100U),
      (long)r->control.target_angle_raw, (long)r->snapshot.feedback.angle_total_raw,
      (long)r->control.angle_error_raw, (long)(r->control.target_speed_rpm * 100.0f),
      r->snapshot.feedback.speed_rpm, r->control.speed_loop_current_raw,
      r->control.gravity_compensation_current_raw, r->control.target_current_raw,
      r->applied_current_raw, r->snapshot.feedback.current_raw,
      r->output_enabled ? 1U : 0U, r->can_submitted ? 1U : 0U,
      r->limit == GM6020_LIMIT_MIN ? "最小" :
          (r->limit == GM6020_LIMIT_MAX ? "最大" : "无"),
      (unsigned long)r->feedback_age_ms,
      r->snapshot_valid && r->snapshot.online ? 1U : 0U,
      r->snapshot_valid ? 1U : 0U, r->snapshot.feedback.temperature_c,
      r->snapshot.feedback.reserved_raw);
  return accepted;
}
