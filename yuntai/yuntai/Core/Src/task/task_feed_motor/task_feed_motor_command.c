/**
 * @file task_feed_motor_command.c
 * @brief 供弹命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 短任务临界区复制全部字段，避免许可、按钮或速度来自不同帧。ISR 不调用接口。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "task/task_feed_motor/task_feed_motor_command.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include <stddef.h>

static FeedMotor_CommandTypeDef g_command; /* DBUS 发布，供弹任务读取；临界区保护。 */
static bool g_received; /* 是否发布过真实命令；时间戳 0 ms 也可能有效。 */

/**
 * @brief 发布一份完整发射命令。
 * @param command 帧时间戳与按钮值；不保留外部指针。
 * @retval true 已发布；false 空指针，旧命令不变。
 * @note 仅 DBUS 任务调用；不创建队列，不阻塞。
 */
bool FeedMotorCommand_Submit(const FeedMotor_CommandTypeDef *command) {
  /* 无效目的/来源指针不进入临界区，不改邮箱或调用者内存。 */
  if (command == NULL) {
    return false;
  }
  /* 按钮、许可和原始接收时刻一起发布；逐字段裸写可能让新按钮配上旧时间。 */
  taskENTER_CRITICAL();
  g_command = *command;
  g_received = true;
  taskEXIT_CRITICAL();
  return true;
}

/**
 * @brief 读取命令并执行 100 ms 超时归零。
 * @param now_ms HAL_GetTick 时间；不能传 FreeRTOS Tick。
 * @param command 输出快照；无命令或过期时两个布尔值为 false。
 * @retval true 有过命令；false 空指针或尚未发布。
 * @note 仅任务调用；短临界区只复制，不等待新帧。
 */
bool FeedMotorCommand_GetSnapshot(uint32_t now_ms, FeedMotor_CommandTypeDef *command) {
  /* 无效目的/来源指针不进入临界区，不改邮箱或调用者内存。 */
  if (command == NULL) {
    return false;
  }
  /* 将按钮、许可、时间戳和首帧标志作为一次状态复制；退出后只处理局部副本。
   * received 与 timestamp 分开，真实 0 ms 帧不能被当作尚未收到命令。 */
  taskENTER_CRITICAL();
  *command = g_command;
  const bool received = g_received;
  taskEXIT_CRITICAL();
  /* 同源 uint32_t 差值允许 HAL ms 回绕。g_received 单独区分未发布与真实 0 ms。
   * 大于半个计数周期的差值按略新的时间处理；要求任务持续运行，不能跨半周期停调度。 */
  const uint32_t age_ms = now_ms - command->timestamp_ms;
  /* 发布者可能抢占并写入略新的同源时间戳；下溢不表示掉线。 */
  if (!received || !command->enabled ||
      (age_ms <= INT32_MAX && age_ms >= FEED_MOTOR_COMMAND_TIMEOUT_MS)) {
    /* 只清除输出副本，保留原邮箱时间；本次查询不能给旧按钮续期。
     * 两个布尔值一起失效，运行时据此立即停止拨弹和故障停轮。 */
    command->enabled = false;
    command->fire_requested = false;
  }
  return received;
}
