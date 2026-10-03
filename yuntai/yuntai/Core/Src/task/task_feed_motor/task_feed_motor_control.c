/**
  ******************************************************************************
  * @file    task_feed_motor_control.c
  * @brief   供弹电机测试状态机实现。
  ******************************************************************************
  */

#include "FreeRTOS.h"

#include "task/task_feed_motor/task_feed_motor_control.h"

#include <stddef.h>

static void FeedMotorControl_SetPhase(FeedMotor_ControlTypeDef *control,
                                      FeedMotor_PhaseTypeDef phase,
                                      uint32_t now_tick) {
  control->phase = phase;
  control->phase_start_tick = now_tick;
}

void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control,
                           uint32_t now_tick) {
  if (control == NULL) {
    return;
  }
  FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_WAIT_FEEDBACK,
                            now_tick);
}

void FeedMotorControl_SetWait(FeedMotor_ControlTypeDef *control,
                              uint32_t now_tick) {
  if (control == NULL) {
    return;
  }
  if (control->phase != FEED_MOTOR_PHASE_WAIT_FEEDBACK) {
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_WAIT_FEEDBACK,
                              now_tick);
  }
}

static uint32_t FeedMotorControl_DurationTicks(
    FeedMotor_PhaseTypeDef phase,
    const FeedMotor_CommandConfigTypeDef *config) {
  uint32_t duration_ms = 0U;
  switch (phase) {
  case FEED_MOTOR_PHASE_UP:
    duration_ms = config->up_time_ms;
    break;
  case FEED_MOTOR_PHASE_DOWN:
    duration_ms = config->down_time_ms;
    break;
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    duration_ms = config->stop_time_ms;
    break;
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  default:
    break;
  }

  /*
   * 配置文件面向人使用毫秒，但阶段起点保存的是 FreeRTOS Tick。
   * 不能直接比较两个单位；即使当前 Tick=1000 Hz，也要保留转换，
   * 否则将来调整 configTICK_RATE_HZ 后，上弹/下弹时间会整体改变。
   */
  return (uint32_t)pdMS_TO_TICKS(duration_ms);
}

bool FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
                             const FeedMotor_CommandConfigTypeDef *config,
                             bool feedback_online,
                             uint32_t now_tick) {
  if (control == NULL || config == NULL) {
    return false;
  }
  if (!feedback_online) {
    FeedMotorControl_SetWait(control, now_tick);
    return false;
  }
  if (control->phase == FEED_MOTOR_PHASE_WAIT_FEEDBACK) {
    /* 第一帧反馈到达后才开始计时，避免把等待时间算进上弹阶段。 */
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_UP, now_tick);
    return true;
  }

  /* 无符号差值可正确处理短时间间隔内的 Tick 回绕。 */
  const uint32_t elapsed_tick = now_tick - control->phase_start_tick;
  const uint32_t duration_tick = FeedMotorControl_DurationTicks(control->phase,
                                                                 config);
  if (elapsed_tick < duration_tick) {
    return false;
  }

  switch (control->phase) {
  case FEED_MOTOR_PHASE_UP:
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_STOP_AFTER_UP,
                              now_tick);
    break;
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_DOWN, now_tick);
    break;
  case FEED_MOTOR_PHASE_DOWN:
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_STOP_AFTER_DOWN,
                              now_tick);
    break;
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_UP, now_tick);
    break;
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  default:
    FeedMotorControl_SetPhase(control, FEED_MOTOR_PHASE_WAIT_FEEDBACK,
                              now_tick);
    break;
  }
  return true;
}

int16_t FeedMotorControl_GetCurrent(
    const FeedMotor_ControlTypeDef *control,
    const FeedMotor_CommandConfigTypeDef *config) {
  if (control == NULL || config == NULL) {
    return 0;
  }
  switch (control->phase) {
  case FEED_MOTOR_PHASE_UP:
    return config->up_current_raw;
  case FEED_MOTOR_PHASE_DOWN:
    return config->down_current_raw;
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
  default:
    return 0;
  }
}

const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
    return "等待反馈";
  case FEED_MOTOR_PHASE_UP:
    return "上弹";
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
    return "上弹停止";
  case FEED_MOTOR_PHASE_DOWN:
    return "下弹";
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    return "下弹停止";
  default:
    return "未知";
  }
}
