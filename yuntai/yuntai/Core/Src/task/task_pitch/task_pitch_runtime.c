/**
  ******************************************************************************
  * @file    task_pitch_runtime.c
  * @brief   Pitch 实测配置与命令来源适配，不复制角度环或保护算法。
  ******************************************************************************
  */
#include "task/task_pitch/task_pitch_runtime.h"
#include "task/task_pitch/task_pitch_config.h"
#include "can.h"
#include <stddef.h>

#define PITCH_TASK_LOG_PERIOD_MS 500U /* 正式模式日志最短间隔，单位 HAL ms。 */

bool PitchTask_RuntimeInit(PitchTask_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  *runtime = (PitchTask_RuntimeTypeDef){0};
  const GimbalAxis_ConfigTypeDef config = {
      .motor = {
          .hcan = &hcan1,
          .feedback_id = GM6020_FEEDBACK_ID_PITCH,
          .feedback_timeout_ms = GM6020_FEEDBACK_TIMEOUT_MS,
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
          .speed_kp_current_per_rpm = PITCH_SPEED_KP_CURRENT_PER_RPM,
          .speed_ki_current_per_rpm_s = PITCH_SPEED_KI_CURRENT_PER_RPM_S,
          .max_current_raw = PITCH_MAX_CURRENT_RAW,
          .current_slew_raw_per_s = PITCH_CURRENT_SLEW_RAW_PER_S,
          .speed_filter_alpha = PITCH_SPEED_FILTER_ALPHA,
      },
      .protection = {
          .startup_grace_ms = PITCH_STARTUP_GRACE_MS,
          .reversal_grace_ms = PITCH_REVERSAL_GRACE_MS,
          .stall_time_ms = PITCH_STALL_TIME_MS,
          .release_confirm_ms = PITCH_RELEASE_CONFIRM_MS,
          .stall_speed_threshold_rpm = PITCH_STALL_SPEED_RPM,
          .stall_command_current_threshold_raw = PITCH_STALL_COMMAND_CURRENT_RAW,
          .stall_feedback_current_threshold_raw = PITCH_STALL_FEEDBACK_CURRENT_RAW,
          .stall_position_delta_raw = PITCH_STALL_POSITION_DELTA_RAW,
          .release_feedback_current_threshold_raw = PITCH_RELEASE_FEEDBACK_CURRENT_RAW,
          .release_position_delta_raw = PITCH_RELEASE_POSITION_DELTA_RAW,
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
  (void)dt_ms;
  /*
   * 关闭硬件测试只表示进入正式入口。Pitch 的真实输入尚未接入，
   * 不能因此自动保持未知角度或施加重力补偿电流。
   */
  (void)now_ms;
  (void)GimbalAxis_SendZero(&runtime->axis);
}
