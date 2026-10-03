/**
  ******************************************************************************
  * @file    task_yaw_command.c
  * @brief   Yaw 输入命令快照和超时处理。
  *
  * 发布者可能来自另一个任务或以后接入的串口/遥控中断，因此用一个轻量
  * 序列号把结构体复制包起来。序列号为奇数时表示正在写入；读取者只接受
  * 前后两次相同的偶数序列号，避免把速度字段和时间字段拼成不同命令。
  ******************************************************************************
  */

#include "task/task_yaw/task_yaw_command.h"

#include "stm32f4xx.h"

#include <stddef.h>

static volatile uint32_t g_yaw_command_sequence;
static Yaw_CommandTypeDef g_yaw_command;

static int16_t YawCommand_ClampVelocity(int16_t velocity_permille) {
  if (velocity_permille < -1000) {
    return -1000;
  }
  if (velocity_permille > 1000) {
    return 1000;
  }
  return velocity_permille;
}

bool YawCommand_Submit(const Yaw_CommandTypeDef *command) {
  if (command == NULL || command->velocity_permille < -1000 ||
      command->velocity_permille > 1000) {
    return false;
  }

  g_yaw_command_sequence++;
  __DMB();
  g_yaw_command = *command;
  g_yaw_command.velocity_permille =
      YawCommand_ClampVelocity(g_yaw_command.velocity_permille);
  __DMB();
  g_yaw_command_sequence++;
  return true;
}

bool YawCommand_GetSnapshot(uint32_t now_ms, Yaw_CommandTypeDef *command) {
  if (command == NULL) {
    return false;
  }

  for (uint8_t retry = 0U; retry < 4U; retry++) {
    const uint32_t sequence_before = g_yaw_command_sequence;
    if ((sequence_before & 1U) != 0U) {
      continue;
    }
    __DMB();
    *command = g_yaw_command;
    __DMB();
    const uint32_t sequence_after = g_yaw_command_sequence;
    if (sequence_before == sequence_after &&
        (sequence_after & 1U) == 0U) {
      if (!command->enabled ||
          now_ms - command->timestamp_ms >= YAW_COMMAND_TIMEOUT_MS) {
        command->enabled = false;
        command->velocity_permille = 0;
      }
      return true;
    }
  }

  command->enabled = false;
  command->velocity_permille = 0;
  command->timestamp_ms = now_ms;
  return false;
}
