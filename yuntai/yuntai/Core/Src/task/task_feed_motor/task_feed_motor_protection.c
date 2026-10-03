/**
  ******************************************************************************
  * @file    task_feed_motor_protection.c
  * @brief   供弹任务的最小 fail-safe 保护实现。
  ******************************************************************************
  */

#include "task/task_feed_motor/task_feed_motor_protection.h"

#include <stddef.h>

void FeedMotorProtection_Update(
    const FeedMotor_ProtectionInputTypeDef *input,
    FeedMotor_ProtectionOutputTypeDef *output) {
  if (output == NULL) {
    return;
  }
  *output = (FeedMotor_ProtectionOutputTypeDef){0};
  if (input == NULL) {
    return;
  }

  output->feedback_lost = !input->feedback_online;
  output->allow_output = input->feedback_online;
}
