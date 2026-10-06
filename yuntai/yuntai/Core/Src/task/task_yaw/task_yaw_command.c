/**
 * @file task_yaw_command.c
 * @brief Yaw命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 单发布者序列号和内存屏障保证一致复制；volatile 本身不保证多字段原子性。
 * 最多重试 4 次；失败返回禁用命令，不能在 ISR 忙等发布者完成。
 * 过期命令停止目标移动；正式 GimbalAxis 按零速度保持位置。
 */

#include "task/task_yaw/task_yaw_command.h"

#include "stm32f4xx.h"

#include <stddef.h>
#include <math.h>

static volatile uint32_t g_yaw_command_sequence; /* 单 DBUS 发布者写序号；Yaw 任务核对，volatile 不替代屏障。 */
static Yaw_CommandTypeDef g_yaw_command; /* DBUS 写值副本，Yaw 读稳定偶数序号覆盖的快照；默认禁用。 */

/* 将相对速度意图限制到 -1000~1000‰；越界输入若直接积分会快速把目标角度推到边界。 */
static float YawCommand_ClampVelocity(float velocity_permille) {
  if (velocity_permille < -1000.0f) {
    return -1000.0f;
  }
  if (velocity_permille > 1000.0f) {
    return 1000.0f;
  }
  return velocity_permille;
}

/**
 * @brief  提交一份新的 Yaw 命令快照（发布者只交出一份值，不交出指针）。
 * @param  command 命令内容；函数只复制结构体，不保存调用者指针（局部变量也可以传入）。
 * @retval true 参数有效并已发布；false 参数为空或速度越界（失败不会更新旧命令）。
 * @note   当前固定由 DBUS 任务单独发布；不阻塞、不打印（序号协议不支持多发布者交错）。
 */
bool YawCommand_Submit(const Yaw_CommandTypeDef *command) {
  /* 先拒绝空指针和范围外速度，失败不修改邮箱或接收时刻。
   * 以下钳位是值复制后的边界防护，不是用来允许非法发布者输入。 */
  if (command == NULL || !isfinite(command->velocity_permille) ||
      command->velocity_permille < -1000.0f ||
      command->velocity_permille > 1000.0f) {
    return false;
  }

  /* 先变奇数，再复制值，最后变偶数。DMB 保持字段写入与序号可见顺序。
   * 仅支持单发布者；volatile 不是锁，多写者会把混合数据标成稳定。（纸条写完才公开。） */
  g_yaw_command_sequence++;
  __DMB();
  g_yaw_command = *command;
  g_yaw_command.velocity_permille =
      YawCommand_ClampVelocity(g_yaw_command.velocity_permille);
  __DMB();
  g_yaw_command_sequence++;
  return true;
}

/**
 * @brief  获取命令并执行超时保护（读取到的是一份完整命令）。
 * @param  now_ms 与提交时间戳相同来源的毫秒时基（不能混用 FreeRTOS Tick）。
 * @param  command 输出命令快照（失败时函数会写入安全的中立命令）。
 * @retval true 读取到一致快照；false 参数为空或发布者正在更新（调用者应保持安全状态）。
 * @note   超时命令 enabled=false、velocity_permille=0，正式运行时按零速度保持位置（有效回中则 enabled=true）。
 */
bool YawCommand_GetSnapshot(uint32_t now_ms, Yaw_CommandTypeDef *command) {
  if (command == NULL) {
    return false;
  }

  /* 有界重试避免输入写入冲突使控制周期长时间停顿；读者不能忙等被抢占的发布者。
   * 默认邮箱 enabled=false，即使序号为 0 且复制成功，也不是有效遥控输入。 */
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
      /* 超时禁用；有效回中为 enabled=true，过期为 false（不能用零速度掩盖断链，运行时停止目标移动并保持位置）。 */
      const uint32_t age_ms = now_ms - command->timestamp_ms;
      /* 发布者可在本轴取时间后抢占并发布略新的帧；这种下溢不能当成超时（同源时钟仍有调用先后竞态）。 */
      if (!command->enabled ||
          (age_ms <= INT32_MAX && age_ms >= YAW_COMMAND_TIMEOUT_MS)) {
        command->enabled = false;
        command->velocity_permille = 0.0f;
      }
      /* true 只表示复制一致；许可可能已因超时清除，调用者仍须按 enabled 处理。 */
      return true;
    }
  }

  /* 多次读不到一致快照也按最安全的中立命令返回，下一周期再重试。 */
  command->enabled = false;
  command->velocity_permille = 0.0f;
  command->timestamp_ms = now_ms;
  return false;
}
