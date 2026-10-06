/**
 * @file task_pitch_command.c
 * @brief Pitch命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 短任务临界区复制全部字段，避免许可、按钮或速度来自不同帧。ISR 不调用接口。
 * 过期命令停止目标移动；正式 GimbalAxis 按零速度保持位置。
 */
#include "task/task_pitch/task_pitch_command.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <math.h>

static Pitch_CommandTypeDef g_pitch_command; /* DBUS 任务写、Pitch 任务读；不暴露可修改指针。 */

/**
 * @brief 发布 Pitch 相对速度命令副本。
 * @param command 速度 ±1000‰，时间戳来自接收帧 HAL ms。
 * @retval true 已发布；false 参数非法，旧命令不变。
 * @note 仅 DBUS 任务调用；短临界区保护完整副本，不阻塞。
 */
bool PitchCommand_Submit(const Pitch_CommandTypeDef *command) {
  if (command == NULL || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f || command->velocity_permille > 1000.0f) {
    return false;
  }
  /* 仅在校验成功后进入短临界区，整体复制速度、许可、固定目标标志和时间戳。
   * 禁止把逐字段写入放到区外，否则读者会把新速度与旧许可一起执行。 */
  taskENTER_CRITICAL();
  g_pitch_command = *command;
  taskEXIT_CRITICAL();
  return true;
}

/**
 * @brief 获取一致命令并处理过期（过期 enabled=false，正式运行时按零速度保持）。
 * @param now_ms HAL 当前 ms，与接收时间同源。
 * @param command 输出副本；禁用时速度为零，回中有效命令则 enabled=true。
 * @retval true 已复制；false 空指针，输出不变。
 * @note 仅任务上下文；读取成功不代表命令有效，必须同时检查 enabled。
 */
bool PitchCommand_GetSnapshot(uint32_t now_ms, Pitch_CommandTypeDef *command) {
  if (command == NULL) {
    return false;
  }
  /* 复制后马上退出临界区；年龄判断不依赖共享内存，不必继续屏蔽 IRQ。
   * 空指针返回 false；复制成功返回 true 仍须检查 enabled，未收到首帧时默认为禁用。 */
  taskENTER_CRITICAL();
  *command = g_pitch_command;
  taskEXIT_CRITICAL();
  /* 不能只把过期速度改零：enabled=false 标记输入失效，运行时停止移动目标并保持；enabled=true 且速度零表示有效回中。 */
  const uint32_t age_ms = now_ms - command->timestamp_ms;
  /* DBUS 高优先级任务可能在 Pitch 取 now_ms 后发布新帧；略新的时间戳不是断链（下溢不能误停）。 */
  if (!command->enabled || (age_ms <= INT32_MAX && age_ms >= PITCH_COMMAND_TIMEOUT_MS)) {
    command->enabled = false;
    command->velocity_permille = 0.0f;
  }
  return true;
}
