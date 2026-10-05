/**
  ******************************************************************************
  * @file    gimbal_control.c
  * @brief   通用云台角度保持控制实现。
  *
  * 通俗理解：相对速度命令先移动目标角度，角度误差产生速度目标，
  * 速度误差再产生电流，最后由 Ramp 限制电流变化速度。
  ******************************************************************************
  */

#include "app/gimbal/gimbal_control.h"

#include <math.h>
#include <stddef.h>

#define GIMBAL_CONTROL_TWO_PI 6.28318530717958647692f

/* 将目标角度、速度或电流限制在同一线性标定范围；不先限幅会让下一环继续追逐不可达目标。 */
static float GimbalControl_Clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

/* 将浮点控制结果转换为 CAN 电流整数；先饱和再转换，避免浮点越界折返成反向大电流。 */
static int16_t GimbalControl_ToCurrent(float current_raw) {
  if (current_raw > 32767.0f) {
    return 32767;
  }
  if (current_raw < -32768.0f) {
    return -32768;
  }
  return (int16_t)current_raw;
}

static float GimbalControl_GravityCurrent(
    const GimbalControl_HandleTypeDef *controller, int32_t angle_raw) {
  if (!controller->config.gravity_compensation_enabled) {
    return 0.0f;
  }
  const float angle_offset_rad =
      ((float)angle_raw - (float)controller->config.center_angle_raw) *
      GIMBAL_CONTROL_TWO_PI / (float)GM6020_ENCODER_COUNTS_PER_REV;
  return controller->config.gravity_compensation_bias_current_raw +
         controller->config.gravity_compensation_amplitude_current_raw *
             cosf(angle_offset_rad);
}

void GimbalControl_Init(GimbalControl_HandleTypeDef *controller,
                     const GimbalControl_ConfigTypeDef *config,
                     int32_t initial_angle_raw) {
  if (controller == NULL || config == NULL ||
      config->min_angle_raw >= config->max_angle_raw ||
      config->max_current_raw <= 0.0f) {
    return;
  }

  controller->config = *config;
  controller->center_angle_raw = config->center_angle_raw;
  controller->target_angle_raw = (float)initial_angle_raw;
  controller->position_integral_rpm = 0.0f;
  controller->target_speed_rpm = 0.0f;
  Pid_Init(&controller->speed_pid, config->speed_kp_current_per_rpm,
           config->speed_ki_current_per_rpm_s, config->speed_kd_current_s_per_rpm,
           -config->max_current_raw, config->max_current_raw,
           -config->speed_integral_limit_raw, config->speed_integral_limit_raw);
  Ramp_Init(&controller->current_ramp, 0.0f);
  LowPassFilter_Init(&controller->speed_filter, 0.0f,
                     config->speed_filter_alpha);
  controller->initialized = true;
}

void GimbalControl_ResetToAngle(GimbalControl_HandleTypeDef *controller,
                             int32_t angle_raw) {
  if (controller == NULL || !controller->initialized) {
    return;
  }
  controller->target_angle_raw = (float)angle_raw;
  controller->position_integral_rpm = 0.0f;
  controller->target_speed_rpm = 0.0f;
  Pid_Reset(&controller->speed_pid);
  Ramp_Reset(&controller->current_ramp, 0.0f);
  LowPassFilter_Reset(&controller->speed_filter, 0.0f);
}

bool GimbalControl_SetTargetAngle(GimbalControl_HandleTypeDef *controller,
                               int32_t target_angle_raw) {
  if (controller == NULL || !controller->initialized) {
    return false;
  }

  controller->target_angle_raw = GimbalControl_Clamp(
      (float)target_angle_raw, (float)controller->config.min_angle_raw,
      (float)controller->config.max_angle_raw);
  return true;
}

void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                          const Gm6020_FeedbackTypeDef *feedback,
                          const Gimbal_CommandTypeDef *command,
                          float dt_s,
                          GimbalControl_OutputTypeDef *output) {
  if (output != NULL) {
    *output = (GimbalControl_OutputTypeDef){0};
  }
  if (controller == NULL || feedback == NULL || command == NULL ||
      output == NULL || !controller->initialized || dt_s <= 0.0f) {
    return;
  }

  const float filtered_speed =
      LowPassFilter_Update(&controller->speed_filter,
                           (float)feedback->speed_rpm);
  const float command_speed = command->enabled
      ? ((float)command->velocity_permille / 1000.0f) *
            controller->config.max_command_speed_rpm
      : 0.0f;
  const bool at_min = feedback->angle_total_raw <= controller->config.min_angle_raw;
  const bool at_max = feedback->angle_total_raw >= controller->config.max_angle_raw;
  const bool remote_command = !command->fixed_target;
  const bool outward_at_min = remote_command && at_min && command_speed <= 0.0f;
  const bool outward_at_max = remote_command && at_max && command_speed >= 0.0f;
  const bool at_or_beyond = at_min || at_max;

  /*
   * 到达或越过软件边界时先把目标重定位到当前反馈位置，再决定是否积分反向输入。
   * 这样回中/外向输入只保持当前点，不会继续追逐越界目标；反向输入从当前点立即
   * 向安全范围移动。若把目标直接钳到边界，轻微越界会立刻产生一股不可调的回拉力，
   * 不能满足“当前位置保持、反向才脱离”的操作语义。
   */
  if (at_or_beyond) {
    const bool target_outward =
        (at_min && controller->target_angle_raw <= feedback->angle_total_raw) ||
        (at_max && controller->target_angle_raw >= feedback->angle_total_raw);
    if (target_outward || (remote_command && (outward_at_min || outward_at_max))) {
      controller->target_angle_raw = (float)feedback->angle_total_raw;
    }
  }
  const bool block_outward = outward_at_min || outward_at_max;
  if (!block_outward && command->enabled) {
    controller->target_angle_raw +=
        command_speed * ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f) * dt_s;
  }
  if (!at_or_beyond) {
    controller->target_angle_raw = GimbalControl_Clamp(
        controller->target_angle_raw, (float)controller->config.min_angle_raw,
        (float)controller->config.max_angle_raw);
  }

  const float angle_error = controller->target_angle_raw -
                            (float)feedback->angle_total_raw;
  const float integral_candidate = controller->position_integral_rpm +
      controller->config.position_ki_rpm_per_raw_s * angle_error * dt_s;
  controller->position_integral_rpm = GimbalControl_Clamp(
      integral_candidate, -controller->config.position_integral_limit_rpm,
      controller->config.position_integral_limit_rpm);
  const float angle_error_rate_raw_per_s =
      (command_speed - filtered_speed) *
      ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f);
  const float target_speed_unclamped =
      controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  if ((target_speed_unclamped > controller->config.max_speed_target_rpm &&
       angle_error > 0.0f) ||
      (target_speed_unclamped < -controller->config.max_speed_target_rpm &&
       angle_error < 0.0f)) {
    controller->position_integral_rpm -=
        controller->config.position_ki_rpm_per_raw_s * angle_error * dt_s;
  }
  float target_speed = controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  target_speed = GimbalControl_Clamp(target_speed,
                                     -controller->config.max_speed_target_rpm,
                                     controller->config.max_speed_target_rpm);
  controller->target_speed_rpm = target_speed;

  const float speed_error = target_speed - filtered_speed;
  const float speed_loop_current =
      Pid_Update(&controller->speed_pid, speed_error, dt_s);
  const float gravity_current = GimbalControl_GravityCurrent(
      controller, feedback->angle_total_raw);
  const float combined_current = GimbalControl_Clamp(
      speed_loop_current + gravity_current, -controller->config.max_current_raw,
      controller->config.max_current_raw);
  const float ramped_current = Ramp_Update(
      &controller->current_ramp, combined_current,
      controller->config.current_slew_raw_per_s, dt_s);

  output->target_angle_raw = (int32_t)controller->target_angle_raw;
  output->angle_error_raw = (int32_t)angle_error;
  output->target_speed_rpm = target_speed;
  output->speed_loop_current_raw = GimbalControl_ToCurrent(speed_loop_current);
  output->gravity_compensation_current_raw =
      GimbalControl_ToCurrent(gravity_current);
  output->target_current_raw = GimbalControl_ToCurrent(ramped_current);
  output->at_min_limit = at_min;
  output->at_max_limit = at_max;
}
