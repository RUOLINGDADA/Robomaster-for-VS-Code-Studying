/**
 * @file task_pitch_command.c
 * @brief Pitch 单发布者命令邮箱（短临界区复制全部字段，避免速度和时间来自不同帧）。
 */
#include "task/task_pitch/task_pitch_command.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>

static Pitch_CommandTypeDef g_pitch_command; /* DBUS 任务写、Pitch 任务读（不能直接暴露可修改指针）。 */

bool PitchCommand_Submit(const Pitch_CommandTypeDef *command) {
  if (command == NULL || command->velocity_permille < -1000 || command->velocity_permille > 1000) {
    return false;
  }
  taskENTER_CRITICAL();
  g_pitch_command = *command;
  taskEXIT_CRITICAL();
  return true;
}

bool PitchCommand_GetSnapshot(uint32_t now_ms, Pitch_CommandTypeDef *command) {
  if (command == NULL) {
    return false;
  }
  taskENTER_CRITICAL();
  *command = g_pitch_command;
  taskEXIT_CRITICAL();
  /* 不能只把过期速度改零：enabled=false 是停机，enabled=true 且速度零是继续闭环保持（掉线不能误当松手）。 */
  const uint32_t age_ms = now_ms - command->timestamp_ms;
  /* DBUS 高优先级任务可能在 Pitch 取 now_ms 后发布新帧；略新的时间戳不是断链（下溢不能误停）。 */
  if (!command->enabled || (age_ms <= INT32_MAX && age_ms >= PITCH_COMMAND_TIMEOUT_MS)) {
    command->enabled = false;
    command->velocity_permille = 0;
  }
  return true;
}
