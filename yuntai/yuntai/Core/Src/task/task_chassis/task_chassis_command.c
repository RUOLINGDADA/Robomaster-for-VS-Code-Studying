/**
 * @file task_chassis_command.c
 * @brief 单 DBUS 发布者的底盘邮箱、Ctrl 事件计数与离线代数。
 *
 * 短 PRIMASK 临界区只复制值和计数，不等待。发布者捕获边沿，消费者掉帧也不丢已发布事件。
 * 此处不判断机械状态，不读 DBUS 硬件，不创建第二个输入任务。
 */
#include "task/task_chassis/task_chassis_command.h"
#include "task/task_chassis/task_chassis_config.h"
#include "stm32f4xx_hal.h"
#include <stddef.h>

static Chassis_CommandTypeDef g_command; /* 单 DBUS 发布者写，底盘原子复制；默认禁用。 */
static bool g_ctrl_down; /* 发布者最近一次 Ctrl 状态，不由消费者改写。 */
static bool g_wait_release = true; /* 上电、掉线和重连先释放 Ctrl，避免持键上线自启动。 */

_Static_assert(CHASSIS_COMMAND_TIMEOUT_MS > 0U && CHASSIS_COMMAND_TIMEOUT_MS <= INT32_MAX,
               "chassis command timeout must fit positive HAL half-range");

/* 限幅使用 int32_t，避免对 INT16_MIN 取反回绕。 */
static int16_t ChassisCommand_Clamp(int16_t raw) {
  return raw > 660 ? 660 : raw < -660 ? -660 : raw;
}

/* 掉线代数使“底盘两次读取间先掉线再上线”仍能重置模式；重复转发不刷新接收时间。 */
bool ChassisCommand_Submit(const Chassis_CommandTypeDef *command) {
  if (command == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  const uint32_t gap = command->timestamp_ms - g_command.timestamp_ms;
  const bool interrupted = g_command.enabled && (!command->enabled ||
      (gap > CHASSIS_COMMAND_TIMEOUT_MS && gap <= INT32_MAX));
  uint32_t generation = g_command.link_generation;
  uint32_t presses = g_command.mode_presses;
  if (interrupted) {
    ++generation;
    presses = 0;
    g_wait_release = true;
    g_ctrl_down = false;
  }
  const bool ctrl = command->enabled && (command->keyboard & (1U << 5U)) != 0U; /* DJI bit5=Ctrl。 */
  if (!command->enabled) {
    g_wait_release = true;
    g_ctrl_down = false;
  } else {
    if (!ctrl) { g_wait_release = false; }
    if (ctrl && !g_ctrl_down && !g_wait_release) { ++presses; }
    g_ctrl_down = ctrl;
  }
  g_command = *command;
  g_command.mode_presses = presses;
  g_command.link_generation = generation;
  g_command.vx_raw = command->enabled ? ChassisCommand_Clamp(command->vx_raw) : 0;
  g_command.vy_raw = command->enabled ? ChassisCommand_Clamp(command->vy_raw) : 0;
  if (!command->enabled) { g_command.keyboard = 0; }
  __set_PRIMASK(mask);
  return true;
}

/* copy 与 now 在任务中比较；短暂未来帧按同源 HAL 重取时间，不把它当巨大超时。 */
bool ChassisCommand_GetSnapshot(uint32_t now_ms, Chassis_CommandTypeDef *command) {
  if (command == NULL) { return false; }
  const uint32_t mask = __get_PRIMASK();
  __disable_irq();
  *command = g_command;
  __set_PRIMASK(mask);
  if (command->enabled && now_ms - command->timestamp_ms > INT32_MAX) { now_ms = HAL_GetTick(); }
  if (!command->enabled || now_ms - command->timestamp_ms > CHASSIS_COMMAND_TIMEOUT_MS) {
    command->enabled = false;
    command->vx_raw = 0;
    command->vy_raw = 0;
    command->keyboard = 0;
  }
  return true;
}
