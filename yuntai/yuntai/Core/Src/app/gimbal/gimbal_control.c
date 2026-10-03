/**
  ******************************************************************************
  * @file    gimbal_control.c
  * @brief   通用云台角度保持控制实现。
  ******************************************************************************
  */

#include "app/gimbal/gimbal_control.h"

#include <stddef.h>

static float GimbalControl_Clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

static int16_t GimbalControl_ToCurrent(float current_raw) {
  if (current_raw > 32767.0f) {
    return 32767;
  }
  if (current_raw < -32768.0f) {
    return -32768;
  }
  return (int16_t)current_raw;
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
  controller->target_speed_rpm = 0.0f;
  Pid_Init(&controller->speed_pid, config->speed_kp_current_per_rpm,
           config->speed_ki_current_per_rpm_s, 0.0f,
           -config->max_current_raw, config->max_current_raw,
           -config->max_current_raw, config->max_current_raw);
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
                       bool hold_output,
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
  const float command_speed =
      command->enabled
          ? ((float)command->velocity_permille / 1000.0f) *
                controller->config.max_command_speed_rpm
          : 0.0f;

  if (!hold_output && command->enabled) {
    /* 速度命令积分为目标角度，dt_s 明确防止把 rpm 当成每周期计数。 */
    controller->target_angle_raw +=
        command_speed * ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f) * dt_s;
  }
  controller->target_angle_raw = GimbalControl_Clamp(
      controller->target_angle_raw, (float)controller->config.min_angle_raw,
      (float)controller->config.max_angle_raw);

  const float angle_error = controller->target_angle_raw -
                            (float)feedback->angle_total_raw;
  const bool pushing_min = command_speed < 0.0f &&
                           feedback->angle_total_raw <=
                               controller->config.min_angle_raw;
  const bool pushing_max = command_speed > 0.0f &&
                           feedback->angle_total_raw >=
                               controller->config.max_angle_raw;
  float target_speed = controller->config.position_kp_rpm_per_raw * angle_error;
  target_speed += command_speed;
  if (pushing_min || pushing_max) {
    target_speed = 0.0f;
  }
  target_speed = GimbalControl_Clamp(target_speed,
                                  -controller->config.max_speed_target_rpm,
                                  controller->config.max_speed_target_rpm);
  controller->target_speed_rpm = target_speed;

  float target_current = 0.0f;
  if (!hold_output && !pushing_min && !pushing_max) {
    const float speed_error = target_speed - filtered_speed;
    target_current = Pid_Update(&controller->speed_pid, speed_error, dt_s);
  } else {
    Pid_Reset(&controller->speed_pid);
  }
  target_current = GimbalControl_Clamp(target_current,
                                    -controller->config.max_current_raw,
                                    controller->config.max_current_raw);
  const float ramped_current = Ramp_Update(
      &controller->current_ramp, target_current,
      controller->config.current_slew_raw_per_s, dt_s);

  output->target_angle_raw = (int32_t)controller->target_angle_raw;
  output->angle_error_raw = (int32_t)angle_error;
  output->target_speed_rpm = target_speed;
  output->target_current_raw = GimbalControl_ToCurrent(ramped_current);
  output->at_min_limit = pushing_min;
  output->at_max_limit = pushing_max;
}


