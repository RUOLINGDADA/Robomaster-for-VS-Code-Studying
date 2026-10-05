/**
  ******************************************************************************
  * @file    task_yaw_command.c
  * @brief   Yaw 输入命令快照和超时处理。
  *
  * 发布者固定为 DBUS 任务，不能有多个发布者交错写入；使用一个轻量
  * 序列号把结构体复制包起来。序列号为奇数时表示正在写入；读取者只接受
  * 前后两次相同的偶数序列号，避免把速度字段和时间字段拼成不同命令。
  ******************************************************************************
  */

#include "task/task_yaw/task_yaw_command.h"

#include "stm32f4xx.h"

#include <stddef.h>

static volatile uint32_t g_yaw_command_sequence;
static Yaw_CommandTypeDef g_yaw_command;

/* 将相对速度意图限制到 -1000~1000‰；越界输入若直接积分会快速把目标角度推到边界。 */
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
    /* 奇数表示发布者正在写；前后序号不一致就丢弃本次复制，避免速度和时间来自不同命令。 */
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
      /* 超时禁用；有效回中为 enabled=true，过期为 false（不能用零速度掩盖断链，运行时会零输出）。 */
      const uint32_t age_ms = now_ms - command->timestamp_ms;
      /* 发布者可在本轴取时间后抢占并发布略新的帧；这种下溢不能当成超时（同源时钟仍有调用先后竞态）。 */
      if (!command->enabled ||
          (age_ms <= INT32_MAX && age_ms >= YAW_COMMAND_TIMEOUT_MS)) {
        command->enabled = false;
        command->velocity_permille = 0;
      }
      return true;
    }
  }

  /* 多次读不到一致快照也按最安全的中立命令返回，下一周期再重试。 */
  command->enabled = false;
  command->velocity_permille = 0;
  command->timestamp_ms = now_ms;
  return false;
}
