/**
  ******************************************************************************
  * @file    task_pitch_runtime.c
  * @brief   Pitch 实测配置与命令来源适配，不复制角度环或保护算法（统一调用 app/gimbal）。
  *
  * 正式输入由 DBUS 发布；未标定或命令失效零输出（不会把固定目标当成遥控命令）。
  ******************************************************************************
  */
#include "task/task_pitch/task_pitch_runtime.h"
#include "task/task_pitch/task_pitch_config.h"
#include "can.h"
#include <stddef.h>
#include "task/task_pitch/task_pitch_command.h"

bool PitchTask_RuntimeInit(PitchTask_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  *runtime = (PitchTask_RuntimeTypeDef){0};
  const GimbalAxis_ConfigTypeDef config = {
      .motor = {
          .hcan = &hcan1,
          .feedback_id = GM6020_FEEDBACK_ID_PITCH,
          .current_sign = PITCH_MOTOR_CURRENT_SIGN,
          .speed_sign = PITCH_FEEDBACK_SPEED_SIGN,
          .feedback_timeout_ms = PITCH_FEEDBACK_TIMEOUT_MS,
      },
      .calibration = {
          .valid = PITCH_CALIBRATION_VALID != 0U,
          .center_angle_raw = PITCH_CALIBRATION_CENTER_ANGLE_RAW,
          .min_angle_raw = PITCH_CALIBRATION_MIN_ANGLE_RAW,
          .max_angle_raw = PITCH_CALIBRATION_MAX_ANGLE_RAW,
      },
      .control = {
          .max_command_speed_rpm = PITCH_MAX_COMMAND_SPEED_RPM,
          .max_speed_target_rpm = PITCH_MAX_SPEED_TARGET_RPM,
          .position_kp_rpm_per_raw = PITCH_POSITION_KP_RPM_PER_RAW,
          .position_ki_rpm_per_raw_s = PITCH_POSITION_KI_RPM_PER_RAW_S,
          .position_integral_limit_rpm = PITCH_POSITION_INTEGRAL_LIMIT_RPM,
          .position_kd_rpm_s_per_raw = PITCH_POSITION_KD_RPM_S_PER_RAW,
          .speed_kp_current_per_rpm = PITCH_SPEED_KP_CURRENT_PER_RPM,
          .speed_ki_current_per_rpm_s = PITCH_SPEED_KI_CURRENT_PER_RPM_S,
          .speed_kd_current_s_per_rpm = PITCH_SPEED_KD_CURRENT_S_PER_RPM,
          .speed_integral_limit_raw = PITCH_SPEED_INTEGRAL_LIMIT_RAW,
          .velocity_feedforward_gain = PITCH_VELOCITY_FEEDFORWARD_GAIN,
          .max_current_raw = PITCH_MAX_CURRENT_RAW,
          .current_slew_raw_per_s = PITCH_CURRENT_SLEW_RAW_PER_S,
          .speed_filter_alpha = PITCH_SPEED_FILTER_ALPHA,
          .gravity_compensation_enabled = true,
          .gravity_compensation_bias_current_raw =
              PITCH_GRAVITY_COMPENSATION_BIAS_CURRENT_RAW,
          .gravity_compensation_amplitude_current_raw =
              PITCH_GRAVITY_COMPENSATION_AMPLITUDE_CURRENT_RAW,
      },
  };
  runtime->calibration_test.axis_name = "发射云台";
  runtime->calibration_test.first_limit_name = "下限位";
  runtime->calibration_test.second_limit_name = "上限位";
  runtime->calibration_test.log_period_ms = PITCH_CALIBRATION_LOG_PERIOD_MS;
  runtime->calibration_test.prompt_period_ms = PITCH_CALIBRATION_PROMPT_PERIOD_MS;
  runtime->angle_test.axis_name = "发射云台角度测试";
  runtime->angle_test.target_angle_deg = PITCH_ANGLE_LOOP_TARGET_ANGLE_DEG;
  runtime->angle_test.log_period_ms = PITCH_ANGLE_LOOP_LOG_PERIOD_MS;
  return GimbalAxis_Init(&runtime->axis, &config);
}

void PitchTask_RuntimeRunCycle(PitchTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms) {
  if (runtime == NULL) {
    return;
  }
  Pitch_CommandTypeDef command = {0};
  (void)PitchCommand_GetSnapshot(now_ms, &command);
  /* 正式模式与 Yaw 共用控制链；命令失效按零速度继续保持当前位置。 */
  GimbalAxis_RunCycle(&runtime->axis, &command, now_ms, dt_ms);
  if (now_ms - runtime->last_log_ms >= PITCH_TASK_LOG_PERIOD_MS) {
    if (GimbalAxis_TryLog(&runtime->axis, "发射云台", command.velocity_permille)) {
      runtime->last_log_ms = now_ms;
    }
  }
}
