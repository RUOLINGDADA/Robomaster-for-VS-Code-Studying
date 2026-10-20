/**
 * @file gimbal_axis.c
 * @brief GM6020 单轴闭环、反馈安全门与周期诊断。
 *
 * Yaw/Pitch 各自独占实例。每周期各取一份 CAN/INS 快照，日志复用它们。
 * CAN2 ISR 更新驱动；任务组合控制、输出许可、聚合帧和 UART 日志。
 * 命令失效保持惯性目标；IMU 失效转编码器保持，CAN/参数失效清零。
 * 本模块不创建任务，不锁存堵转，不执行机械急停。
 */
#include "app/gimbal/gimbal_axis.h"
#include <math.h>
#include <stddef.h>

#define GIMBAL_AXIS_DEG_TO_RAD 0.017453292519943295f /* deg 转 rad，用于 ZYX 欧拉角速度关系。 */
#define GIMBAL_AXIS_RAD_S_TO_RPM 9.54929658551372f /* rad/s 转 rpm，60/(2π)。 */
#define GIMBAL_AXIS_RAW_PER_DEG ((float)GM6020_ENCODER_COUNTS_PER_REV / 360.0f) /* 8192 count/rev 对应的等效 count/deg。 */

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

/* 只验证启用的惯性配置；机械角测试和未启用自稳的调用者不需要填写这些字段。 */
static bool GimbalAxis_CheckStabilization(const GimbalAxis_StabilizationConfigTypeDef *c) {
  return !c->enabled ||
      ((c->axis == GIMBAL_AXIS_INERTIAL_YAW || c->axis == GIMBAL_AXIS_INERTIAL_PITCH) &&
       (c->feedback_sign == 1 || c->feedback_sign == -1) &&
       c->ready_ms > 0U && c->ready_ms <= INT32_MAX &&
       c->timeout_ms > 0U && c->timeout_ms < c->ready_ms &&
       isfinite(c->exit_pitch_deg) && c->exit_pitch_deg > 0.0f && c->exit_pitch_deg < 90.0f &&
       isfinite(c->enter_pitch_deg) && c->enter_pitch_deg > 0.0f && c->enter_pitch_deg < c->exit_pitch_deg);
}

/* 参数无效时禁止闭环，避免 NaN、越界增益或错误滤波权重进入电流计算。 */
static bool GimbalAxis_CheckParameters(const GimbalAxis_ConfigTypeDef *config) {
  const GimbalControl_ConfigTypeDef *c = &config->control;
  GravityCompensation_HandleTypeDef gravity = {0};
  /* 模型合法性由算法模块定义；这里仅核对补偿上限不超过轴总电流上限。 */
  return GimbalAxis_CheckStabilization(&config->stabilization) &&
         GravityCompensation_Init(&gravity, &c->gravity_compensation) &&
         isfinite(c->position_kp_rpm_per_raw) &&
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
         c->gravity_compensation.max_current_raw <= c->max_current_raw &&
         c->speed_integral_limit_raw <= c->max_current_raw &&
         c->position_integral_limit_rpm <= c->max_speed_target_rpm;
}

/**
 * @brief 复制配置并注册独立 GM6020 轴。
 * @param axis 持久轴对象；每个任务独占一个。
 * @param config CAN、标定和控制参数；先完成板级 CAN 初始化。
 * @retval true 已注册；false 空指针或驱动注册失败。标定/增益许可另存于对象。
 * @note 仅所属任务首次调用。注册成功不代表已收到反馈或允许闭环。
 */
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

/**
 * @brief 关闭本轴输出许可并提交零电流聚合帧。
 * @param axis 已注册轴对象。
 * @retval true HAL 已接受帧；false 句柄无效、邮箱忙或 HAL 失败。
 * @note 仅所属任务调用。失败仍保留零目标；提交成功不证明电调已停止。
 */
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

/**
 * @brief 取得阶段中文名称。
 * @param phase 当前阶段枚举；未知值按初始化描述。
 * @retval 静态只读字符串；不能修改或释放。
 * @note 纯查询，无硬件或共享状态操作。
 */
const char *GimbalAxis_PhaseName(GimbalAxis_PhaseTypeDef phase) {
  switch (phase) {
  case GIMBAL_AXIS_PHASE_WAIT_FEEDBACK: return "等待反馈";
  case GIMBAL_AXIS_PHASE_HOLD_POSITION: return "保持位置";
  case GIMBAL_AXIS_PHASE_ACTIVE: return "主动控制";
  case GIMBAL_AXIS_PHASE_LIMIT_HOLD: return "人工命令受限，闭环保持";
  case GIMBAL_AXIS_PHASE_FEEDBACK_LOST: return "反馈丢失";
  default: return "初始化";
  }
}

/**
 * @brief 取得诊断原因中文名称。
 * @param reason 当前原因枚举；未知值按无原因描述。
 * @retval 静态只读字符串；不能修改或释放。
 * @note 纯查询，无硬件或共享状态操作。
 */
const char *GimbalAxis_ReasonName(GimbalAxis_ReasonTypeDef reason) {
  switch (reason) {
  case GIMBAL_AXIS_REASON_SOFT_MIN: return "软件最小限位";
  case GIMBAL_AXIS_REASON_SOFT_MAX: return "软件最大限位";
  case GIMBAL_AXIS_REASON_FEEDBACK_LOST: return "反馈丢失";
  case GIMBAL_AXIS_REASON_CALIBRATION_INVALID: return "未完成角度标定";
  case GIMBAL_AXIS_REASON_TARGET_INVALID: return "目标无效或越界";
  case GIMBAL_AXIS_REASON_CONTROL_INVALID: return "控制参数无效";
  case GIMBAL_AXIS_REASON_INPUT_HOLD: return "遥控输入保持";
  case GIMBAL_AXIS_REASON_IMU_WAIT: return "等待惯性反馈，编码器保持";
  case GIMBAL_AXIS_REASON_IMU_INVALID: return "惯性反馈无效，编码器保持";
  case GIMBAL_AXIS_REASON_IMU_RESET: return "惯性参考已重置，编码器保持";
  case GIMBAL_AXIS_REASON_IMU_SINGULAR: return "惯性倾角保护，编码器保持";
  default: return "无";
  }
}

/* 诊断仅显示本周期实际参考系，不能把编码器保持误报为惯性自稳。 */
const char *GimbalAxis_FeedbackName(GimbalAxis_FeedbackSourceTypeDef source) {
  return source == GIMBAL_AXIS_FEEDBACK_INERTIAL ? "六轴惯性" : "机械编码器";
}

/* 无效姿态只请求一次反馈切换；CAN 离线时延迟捕获机械角，不能复用旧编码器值。 */
static bool GimbalAxis_RejectImu(GimbalAxis_HandleTypeDef *axis,
                                GimbalAxis_ReasonTypeDef reason) {
  axis->imu_qualifying = false;
  axis->fallback_pending = axis->fallback_pending ||
      axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL;
  axis->cycle.reason = reason;
  return false;
}

/* 一次复制 INS 并判定参考连续性，再把 p/q/r 换成与欧拉目标一致的角速度。
 * 靠近 Pitch ±90°时不能除 cos(Pitch)。无效/重置先降级，再累计新的有效窗口。
 * 同序号快照不能完成进入窗口；Getter 还会在 INS 任务停摆时撤销有效性。 */
static bool GimbalAxis_ObserveImu(GimbalAxis_HandleTypeDef *axis,
                                 uint32_t now_ms,
                                 GimbalControl_FeedbackTypeDef *feedback) {
  const GimbalAxis_StabilizationConfigTypeDef *c = &axis->config.stabilization;
  Ins_SnapshotTypeDef *s = &axis->cycle.ins;
  axis->cycle.ins_available = Ins_GetSnapshot(s);
  axis->cycle.gyro_age_ms = GimbalAxis_FeedbackAge(now_ms, s->gyro_ms);
  axis->cycle.accel_age_ms = GimbalAxis_FeedbackAge(now_ms, s->accel_ms);
  if (!axis->cycle.ins_available || s->state != INS_STATE_READY || !s->attitude_valid ||
      !s->accel_valid || axis->cycle.gyro_age_ms > c->timeout_ms ||
      axis->cycle.accel_age_ms > c->timeout_ms ||
      !isfinite(s->yaw_total_deg) || !isfinite(s->pitch_deg) || !isfinite(s->roll_deg) ||
      !isfinite(s->gyro_rad_s[0]) || !isfinite(s->gyro_rad_s[1]) || !isfinite(s->gyro_rad_s[2])) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_INVALID);
  }
  const float pitch_abs_deg = fabsf(s->pitch_deg);
  if (pitch_abs_deg >= c->exit_pitch_deg ||
      (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_ENCODER && pitch_abs_deg > c->enter_pitch_deg)) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_SINGULAR);
  }
  if (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL &&
      s->reset_sequence != axis->imu_active_reset) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_RESET);
  }

  const float roll_rad = s->roll_deg * GIMBAL_AXIS_DEG_TO_RAD;
  const float pitch_rad = s->pitch_deg * GIMBAL_AXIS_DEG_TO_RAD;
  const float sin_roll = sinf(roll_rad), cos_roll = cosf(roll_rad);
  const float rate_rad_s = c->axis == GIMBAL_AXIS_INERTIAL_YAW
      ? (s->gyro_rad_s[1] * sin_roll + s->gyro_rad_s[2] * cos_roll) / cosf(pitch_rad)
      : s->gyro_rad_s[1] * cos_roll - s->gyro_rad_s[2] * sin_roll;
  const float angle_deg = c->axis == GIMBAL_AXIS_INERTIAL_YAW ? s->yaw_total_deg : s->pitch_deg;
  feedback->angle_raw = angle_deg * GIMBAL_AXIS_RAW_PER_DEG * (float)c->feedback_sign;
  feedback->speed_rpm = rate_rad_s * GIMBAL_AXIS_RAD_S_TO_RPM * (float)c->feedback_sign;
  feedback->mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw;
  if (!isfinite(feedback->angle_raw) || !isfinite(feedback->speed_rpm)) {
    return GimbalAxis_RejectImu(axis, GIMBAL_AXIS_REASON_IMU_INVALID);
  }
  if (axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL && !axis->fallback_pending) {
    return true;
  }
  axis->cycle.reason = GIMBAL_AXIS_REASON_IMU_WAIT;
  if (axis->fallback_pending) { return false; }
  if (!axis->imu_qualifying || s->reset_sequence != axis->imu_candidate_reset) {
    axis->imu_qualifying = true;
    axis->imu_ready_start_ms = now_ms;
    axis->imu_last_sequence = s->sequence;
    axis->imu_candidate_reset = s->reset_sequence;
    return false;
  }
  const bool sample_advanced = s->sequence != axis->imu_last_sequence;
  axis->imu_last_sequence = s->sequence;
  return sample_advanced && (uint32_t)(now_ms - axis->imu_ready_start_ms) >= c->ready_ms;
}

/* 清理旧积分和 Ramp，但保留最后目标；恢复后重新计算目标误差，不自动改位。 */
static void GimbalAxis_ResetController(GimbalAxis_HandleTypeDef *axis) {
  GimbalControl_ResetHistory(&axis->controller);
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
  axis->cycle.feedback_source = axis->feedback_source;
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
  const bool stabilization = axis->config.stabilization.enabled && !fixed_target;
  GimbalControl_FeedbackTypeDef inertial_feedback = {0};
  const bool inertial_ready = stabilization &&
      GimbalAxis_ObserveImu(axis, now_ms, &inertial_feedback);

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
  if (command == NULL || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f ||
      command->velocity_permille > 1000.0f) {
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
    const GimbalControl_FeedbackTypeDef initial_feedback = {
        .angle_raw = (float)axis->cycle.snapshot.feedback.angle_total_raw,
        .speed_rpm = (float)axis->cycle.snapshot.feedback.speed_rpm,
        .mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw,
    };
    (void)GimbalControl_Reanchor(&axis->controller, &initial_feedback);
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

  GimbalControl_FeedbackTypeDef feedback = {
      .angle_raw = (float)axis->cycle.snapshot.feedback.angle_total_raw,
      .speed_rpm = (float)axis->cycle.snapshot.feedback.speed_rpm,
      .mechanical_angle_raw = axis->cycle.snapshot.feedback.angle_total_raw,
  };
  /* 参考切换只发生在状态边沿。范围外也锁住当前反馈，不能用 min/max 裁剪。
   * CAN 恢复且 INS 参考连续时保留惯性目标；INS 失效则捕获机械角并暂停人工输入。
   * 切换周期不积分输入，避免刚锁定就因旧命令发生目标跳变。 */
  if (axis->fallback_pending || (!stabilization && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL)) {
    (void)GimbalControl_Reanchor(&axis->controller, &feedback);
    axis->feedback_source = GIMBAL_AXIS_FEEDBACK_ENCODER;
    axis->fallback_pending = false;
    axis->imu_qualifying = false;
    axis->cycle.input_inhibited = true;
  } else if (stabilization && inertial_ready && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_ENCODER) {
    (void)GimbalControl_Reanchor(&axis->controller, &inertial_feedback);
    axis->feedback_source = GIMBAL_AXIS_FEEDBACK_INERTIAL;
    axis->imu_active_reset = axis->cycle.ins.reset_sequence;
    axis->imu_qualifying = false;
    axis->cycle.reason = GIMBAL_AXIS_REASON_NONE;
    axis->cycle.input_inhibited = true;
  }
  if (stabilization && axis->feedback_source == GIMBAL_AXIS_FEEDBACK_INERTIAL) {
    feedback = inertial_feedback;
  } else if (stabilization) {
    axis->cycle.input_inhibited = true;
  }
  axis->cycle.feedback_source = axis->feedback_source;
  const Gimbal_CommandTypeDef hold_command = {
      .enabled = true, .velocity_permille = 0.0f, .timestamp_ms = now_ms,
  };
  const Gimbal_CommandTypeDef *effective_command = axis->cycle.input_inhibited ? &hold_command : command;
  if (fixed_target) {
    (void)GimbalControl_SetTargetAngle(&axis->controller, target_angle_raw);
  }
  GimbalControl_UpdateFeedback(&axis->controller, &feedback,
      effective_command, (float)dt_ms / 1000.0f, &axis->cycle.control);
  if (!axis->cycle.control.valid) {
    GimbalAxis_ResetController(axis);
    axis->cycle.reason = GIMBAL_AXIS_REASON_CONTROL_INVALID;
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (axis->cycle.control.command_limited && effective_command->velocity_permille < 0.0f) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MIN;
  } else if (axis->cycle.control.command_limited) {
    axis->cycle.reason = GIMBAL_AXIS_REASON_SOFT_MAX;
  } else if (axis->cycle.reason == GIMBAL_AXIS_REASON_NONE &&
             (!effective_command->enabled || effective_command->velocity_permille == 0.0f)) {
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

  if (axis->cycle.control.command_limited) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_LIMIT_HOLD;
  } else if (axis->cycle.control.accepted_command_speed_rpm != 0.0f) {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_ACTIVE;
  } else {
    axis->cycle.phase = GIMBAL_AXIS_PHASE_HOLD_POSITION;
  }
}

/**
 * @brief 按相对速度推进一个正式闭环周期。
 * @param axis 已注册轴对象。
 * @param command 速度 ±1000‰；空、失效或越界时按零速度保持。
 * @param now_ms HAL_GetTick 当前 ms；与反馈时间戳同源。
 * @param dt_ms 控制周期 ms；必须大于 0。
 * @retval None；周期输出写入 axis->cycle，反馈或参数失效时发零电流。
 * @note 仅所属任务调用。每周期各取一份 CAN/INS 快照，不等待 CAN 发送完成。
 * 启用自稳时使用惯性角；IMU 未就绪/失效时捕获编码器保持并暂停人工输入。
 */
void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms) {
  /* 命令失效或数值越界等价于零速度保持；超时由命令适配层处理。
   * CAN、标定或参数失效仍发零；IMU 失效保持编码器位置。 */
  const Gimbal_CommandTypeDef hold_command = {
      .velocity_permille = 0, .enabled = true, .fixed_target = false,
      .timestamp_ms = now_ms};
  const bool command_valid = command != NULL && command->enabled &&
                             isfinite(command->velocity_permille) &&
                             command->velocity_permille >= -1000.0f &&
                             command->velocity_permille <= 1000.0f;
  GimbalAxis_Run(axis, command_valid ? command : &hold_command,
                 false, 0, now_ms, dt_ms);
}

/**
 * @brief 使用正式控制器保持固定角度。
 * @param axis 已注册轴对象。
 * @param target_angle_raw 标定范围内的连续 count；越界发零电流。
 * @param now_ms HAL 当前 ms；用于反馈年龄。
 * @param dt_ms 控制周期 ms；必须大于 0。
 * @retval None；周期结果写入 axis->cycle。
 * @note 仅所属任务调用。固定目标和相对速度模式同周期不能同时执行。
 */
void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms) {
  const Gimbal_CommandTypeDef fixed_command = {
      .velocity_permille = 0, .enabled = true, .fixed_target = true,
      .timestamp_ms = now_ms};
  GimbalAxis_Run(axis, &fixed_command, true, target_angle_raw, now_ms, dt_ms);
}
