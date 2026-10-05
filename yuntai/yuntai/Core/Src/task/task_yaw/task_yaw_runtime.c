/**
  ******************************************************************************
  * @file    task_yaw_runtime.c
  * @brief   Yaw 实测配置与命令来源适配，不复制角度环或保护算法。
  *
  * 通俗理解：这里负责把 Yaw 的配置和输入接线，控制算法仍由 app/gimbal 统一完成。
  ******************************************************************************
  */
#include "task/task_yaw/task_yaw_runtime.h"
#include "task/task_yaw/task_yaw_config.h"
#include "can.h"
#include <stddef.h>
#include "task/task_yaw/task_yaw_command.h"

bool YawTask_RuntimeInit(YawTask_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  *runtime = (YawTask_RuntimeTypeDef){0};
  const GimbalAxis_ConfigTypeDef config = {
      .motor = {
          .hcan = &hcan1,
          .feedback_id = GM6020_FEEDBACK_ID_YAW,
          .current_sign = YAW_MOTOR_CURRENT_SIGN,
          .speed_sign = YAW_FEEDBACK_SPEED_SIGN,
          .feedback_timeout_ms = YAW_FEEDBACK_TIMEOUT_MS,
      },
      .calibration = {
          .valid = YAW_CALIBRATION_VALID != 0U,
          .center_angle_raw = YAW_CALIBRATION_CENTER_ANGLE_RAW,
          .min_angle_raw = YAW_CALIBRATION_MIN_ANGLE_RAW,
          .max_angle_raw = YAW_CALIBRATION_MAX_ANGLE_RAW,
      },
      .control = {
          .max_command_speed_rpm = YAW_MAX_COMMAND_SPEED_RPM,
          .max_speed_target_rpm = YAW_MAX_SPEED_TARGET_RPM,
          .position_kp_rpm_per_raw = YAW_POSITION_KP_RPM_PER_RAW,
          .position_ki_rpm_per_raw_s = YAW_POSITION_KI_RPM_PER_RAW_S,
          .position_integral_limit_rpm = YAW_POSITION_INTEGRAL_LIMIT_RPM,
          .position_kd_rpm_s_per_raw = YAW_POSITION_KD_RPM_S_PER_RAW,
          .speed_kp_current_per_rpm = YAW_SPEED_KP_CURRENT_PER_RPM,
          .speed_ki_current_per_rpm_s = YAW_SPEED_KI_CURRENT_PER_RPM_S,
          .speed_kd_current_s_per_rpm = YAW_SPEED_KD_CURRENT_S_PER_RPM,
          .speed_integral_limit_raw = YAW_SPEED_INTEGRAL_LIMIT_RAW,
          .velocity_feedforward_gain = YAW_VELOCITY_FEEDFORWARD_GAIN,
          .max_current_raw = YAW_MAX_CURRENT_RAW,
          .current_slew_raw_per_s = YAW_CURRENT_SLEW_RAW_PER_S,
          .speed_filter_alpha = YAW_SPEED_FILTER_ALPHA,
          .gravity_compensation_enabled = false,
          .gravity_compensation_bias_current_raw = 0.0f,
          .gravity_compensation_amplitude_current_raw = 0.0f,
      },
  };
  runtime->calibration_test.axis_name = "水平轴";
  runtime->calibration_test.first_limit_name = "左限位";
  runtime->calibration_test.second_limit_name = "右限位";
  runtime->calibration_test.log_period_ms = YAW_CALIBRATION_LOG_PERIOD_MS;
  runtime->calibration_test.prompt_period_ms = YAW_CALIBRATION_PROMPT_PERIOD_MS;
  runtime->angle_test.axis_name = "水平轴角度测试";
  runtime->angle_test.target_angle_deg = YAW_ANGLE_LOOP_TARGET_ANGLE_DEG;
  runtime->angle_test.log_period_ms = YAW_ANGLE_LOOP_LOG_PERIOD_MS;
  return GimbalAxis_Init(&runtime->axis, &config);
}

void YawTask_RuntimeRunCycle(YawTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms) {
  if (runtime == NULL) {
    return;
  }
  Yaw_CommandTypeDef command = {0};
  (void)YawCommand_GetSnapshot(now_ms, &command);
  GimbalAxis_RunCycle(&runtime->axis, &command, now_ms, dt_ms);
  if (now_ms - runtime->last_log_ms >= YAW_TASK_LOG_PERIOD_MS) {
    if (GimbalAxis_TryLog(&runtime->axis, "水平轴", command.velocity_permille)) {
      runtime->last_log_ms = now_ms;
    }
  }
}
