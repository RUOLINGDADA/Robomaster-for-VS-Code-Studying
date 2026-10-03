/**
  ******************************************************************************
  * @file    gimbal_protection.c
  * @brief   通用轴保护实现：连续证据确认、方向相关释放、掉线保留堵转。
  ******************************************************************************
  */
#include "app/gimbal/gimbal_protection.h"
#include <stddef.h>

static int32_t GimbalProtection_AbsI16(int16_t value) {
  const int32_t wide = value; /* 先扩大到 32 位，避免对 -32768 求绝对值溢出。 */
  return wide < 0 ? -wide : wide;
}

static int8_t GimbalProtection_Sign(int16_t value) {
  return value > 0 ? 1 : (value < 0 ? -1 : 0);
}

static void GimbalProtection_ClearCandidate(GimbalProtection_HandleTypeDef *p) {
  p->candidate_active = false;
  p->candidate_start_ms = 0U;
}

void GimbalProtection_Init(GimbalProtection_HandleTypeDef *p,
                            const GimbalProtection_ConfigTypeDef *config) {
  if (p == NULL || config == NULL) {
    return;
  }
  *p = (GimbalProtection_HandleTypeDef){0};
  p->config = *config;
  p->guard_duration_ms = config->startup_grace_ms;
}

void GimbalProtection_OnFeedbackLost(GimbalProtection_HandleTypeDef *p) {
  if (p == NULL) {
    return;
  }
  GimbalProtection_ClearCandidate(p);
  p->release_active = false; /* 掉线期间没有连续证据，恢复确认必须重新计时。 */
  p->output_was_enabled = false;
  p->last_drive_direction = 0;
  p->last_command_direction = 0;
  if (p->reason == GIMBAL_PROTECTION_NONE) {
    p->reason = GIMBAL_PROTECTION_FEEDBACK_LOST;
  }
}

const char *GimbalProtection_ReasonName(GimbalProtection_ReasonTypeDef reason) {
  switch (reason) {
  case GIMBAL_PROTECTION_SOFT_MIN: return "软件最小限位";
  case GIMBAL_PROTECTION_SOFT_MAX: return "软件最大限位";
  case GIMBAL_PROTECTION_STALL_HOLD: return "堵转保持";
  case GIMBAL_PROTECTION_FEEDBACK_LOST: return "反馈丢失";
  case GIMBAL_PROTECTION_CALIBRATION_INVALID: return "未完成角度标定";
  case GIMBAL_PROTECTION_TARGET_INVALID: return "目标无效或越界";
  case GIMBAL_PROTECTION_CONTROL_INVALID: return "控制参数无效";
  default: return "无";
  }
}

/*
 * 固定目标的释放看真实位移，正式相对控制的释放看操作者松手/反向意图。
 * 两种模式共享电流、范围、新鲜度和持续时间条件，不能由测试自行清保护。
 */
static bool GimbalProtection_ReleaseRequested(
    const GimbalProtection_HandleTypeDef *p,
    const GimbalProtection_InputTypeDef *input, int8_t command_direction) {
  const bool in_range = input->angle_total_raw > p->config.min_angle_raw &&
                        input->angle_total_raw < p->config.max_angle_raw;
  if (p->stall_requires_manual_release) {
    const int64_t moved = (int64_t)input->angle_total_raw -
                          p->stall_trigger_angle_raw;
    return in_range && p->blocked_direction != 0 &&
           p->config.release_position_delta_raw > 0U &&
           (int64_t)p->blocked_direction * moved <=
               -(int64_t)p->config.release_position_delta_raw;
  }
  const bool inward_allowed =
      !((input->angle_total_raw <= p->config.min_angle_raw &&
         command_direction < 0) ||
        (input->angle_total_raw >= p->config.max_angle_raw &&
         command_direction > 0));
  return inward_allowed &&
         (command_direction == 0 || command_direction != p->blocked_direction);
}

void GimbalProtection_Update(GimbalProtection_HandleTypeDef *p,
                              const GimbalProtection_InputTypeDef *input,
                              GimbalProtection_OutputTypeDef *output) {
  if (output != NULL) {
    *output = (GimbalProtection_OutputTypeDef){0}; /* 默认禁止输出。 */
  }
  if (p == NULL || input == NULL || output == NULL) {
    return;
  }
  if (!input->feedback_online) {
    GimbalProtection_OnFeedbackLost(p);
    goto finished;
  }
  if (p->reason == GIMBAL_PROTECTION_FEEDBACK_LOST) {
    p->reason = GIMBAL_PROTECTION_NONE;
  }

  const int8_t command_direction =
      GimbalProtection_Sign(input->command_velocity_permille);
  const int8_t current_direction =
      GimbalProtection_Sign(input->requested_current_raw);

  if (p->reason == GIMBAL_PROTECTION_STALL_HOLD) {
    const bool low_current =
        GimbalProtection_AbsI16(input->filtered_current_raw) <=
        p->config.release_feedback_current_threshold_raw;
    if (low_current &&
        GimbalProtection_ReleaseRequested(p, input, command_direction)) {
      if (!p->release_active) {
        p->release_active = true;
        p->release_start_ms = input->now_ms;
      }
      if (input->now_ms - p->release_start_ms >=
          p->config.release_confirm_ms) {
        p->reason = GIMBAL_PROTECTION_NONE;
        p->blocked_direction = 0;
        p->release_active = false;
        p->output_was_enabled = false;
        p->last_drive_direction = 0;
        p->guard_start_ms = input->now_ms;
        p->guard_duration_ms = p->config.startup_grace_ms;
        GimbalProtection_ClearCandidate(p);
        output->cleared = true; /* 本周期仍为零；下一周期从清零后的 Ramp 开始。 */
      }
    } else {
      p->release_active = false;
    }
    goto finished;
  }

  /*
   * 软件边界按运动意图和实际电流方向处理：向外停止，向内才解除。
   * 固定目标的意图来自位置误差，不能始终使用一个“零速度命令”。
   */
  if (p->reason == GIMBAL_PROTECTION_SOFT_MIN ||
      p->reason == GIMBAL_PROTECTION_SOFT_MAX) {
    const bool inward =
        (p->reason == GIMBAL_PROTECTION_SOFT_MIN && command_direction > 0) ||
        (p->reason == GIMBAL_PROTECTION_SOFT_MAX && command_direction < 0);
    GimbalProtection_ClearCandidate(p);
    if (inward) {
      p->reason = GIMBAL_PROTECTION_NONE;
      output->cleared = true;
      p->output_was_enabled = false;
    }
    goto finished;
  }
  if (input->angle_total_raw <= p->config.min_angle_raw &&
      (command_direction < 0 || current_direction < 0)) {
    p->reason = GIMBAL_PROTECTION_SOFT_MIN;
    GimbalProtection_ClearCandidate(p);
    goto finished;
  }
  if (input->angle_total_raw >= p->config.max_angle_raw &&
      (command_direction > 0 || current_direction > 0)) {
    p->reason = GIMBAL_PROTECTION_SOFT_MAX;
    GimbalProtection_ClearCandidate(p);
    goto finished;
  }

  const bool restarted = input->output_enabled && !p->output_was_enabled;
  const bool command_reversed = command_direction != 0 &&
                                 command_direction != p->last_command_direction;
  const bool drive_reversed = current_direction != 0 &&
                               current_direction != p->last_drive_direction;
  if (restarted || command_reversed || drive_reversed) {
    p->guard_start_ms = input->now_ms;
    p->guard_duration_ms = restarted ? p->config.startup_grace_ms
                                     : p->config.reversal_grace_ms;
    GimbalProtection_ClearCandidate(p);
  }
  p->output_was_enabled = input->output_enabled;
  p->last_command_direction = command_direction;
  if (current_direction != 0) {
    p->last_drive_direction = current_direction;
  }

  const bool evidence =
      input->output_enabled && current_direction != 0 &&
      input->now_ms - p->guard_start_ms >= p->guard_duration_ms &&
      (command_direction == 0 || command_direction == current_direction) &&
      GimbalProtection_AbsI16(input->requested_current_raw) >=
          p->config.stall_command_current_threshold_raw &&
      GimbalProtection_AbsI16(input->filtered_current_raw) >=
          p->config.stall_feedback_current_threshold_raw &&
      GimbalProtection_AbsI16(input->filtered_speed_rpm) <=
          p->config.stall_speed_threshold_rpm;
  if (!evidence) {
    GimbalProtection_ClearCandidate(p);
  } else {
    if (!p->candidate_active) {
      p->candidate_active = true;
      p->candidate_start_ms = input->now_ms;
      p->candidate_angle_raw = input->angle_total_raw;
    }
    const int64_t moved = (int64_t)input->angle_total_raw -
                          p->candidate_angle_raw;
    if ((moved < 0 ? -moved : moved) > p->config.stall_position_delta_raw) {
      p->candidate_start_ms = input->now_ms;
      p->candidate_angle_raw = input->angle_total_raw;
    } else if (input->now_ms - p->candidate_start_ms >= p->config.stall_time_ms) {
      p->reason = GIMBAL_PROTECTION_STALL_HOLD;
      p->blocked_direction = current_direction;
      p->stall_trigger_angle_raw = input->angle_total_raw;
      p->stall_requires_manual_release = input->fixed_target;
      p->release_active = false;
      output->entered = true;
    }
  }
  output->allow_output = p->reason == GIMBAL_PROTECTION_NONE;

finished:
  /* 一个出口同步事件、原因和方向，避免触发日志丢掉本周期的新方向。 */
  output->reason = p->reason;
  output->blocked_direction = p->blocked_direction;
}
