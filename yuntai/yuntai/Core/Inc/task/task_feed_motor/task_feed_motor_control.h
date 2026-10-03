/**
  ******************************************************************************
  * @file    task_feed_motor_control.h
  * @brief   供弹电机上弹/停止/下弹/停止测试状态机。
  *
  * 控制层只计算当前阶段和目标电流，不直接访问 C610、CAN、FreeRTOS 或
  * 串口。这样阶段顺序可以在主机上单独验证，也避免任务入口混入业务状态。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_CONTROL_H
#define TASK_FEED_MOTOR_CONTROL_H /* 防止状态机接口重复包含。 */

#include "task/task_feed_motor/task_feed_motor_command.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  FEED_MOTOR_PHASE_WAIT_FEEDBACK = 0, /* 未收到有效反馈，强制零输出。 */
  FEED_MOTOR_PHASE_UP,                /* 使用正方向电流测试上弹。 */
  FEED_MOTOR_PHASE_STOP_AFTER_UP,     /* 上弹后卸力，避免立即反向冲击。 */
  FEED_MOTOR_PHASE_DOWN,              /* 使用负方向电流测试下弹。 */
  FEED_MOTOR_PHASE_STOP_AFTER_DOWN    /* 下弹后卸力，再回到上弹。 */
} FeedMotor_PhaseTypeDef;

typedef struct {
  FeedMotor_PhaseTypeDef phase;
  uint32_t phase_start_tick; /* FreeRTOS Tick，不能与 HAL 毫秒直接相减。 */
} FeedMotor_ControlTypeDef;

/**
 * @brief  将供弹测试状态机置于等待反馈阶段。
 * @param  control 任务独占的控制对象，不能为 NULL。
 * @param  now_tick 当前 FreeRTOS Tick，作为等待阶段新的起点。
 * @note   这里只清阶段，不访问 CAN，也不发送电流；硬件安全动作由运行时层完成。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control,
                           uint32_t now_tick);

/**
 * @brief  在反馈失联时回到等待阶段。
 * @param  control 任务独占的控制对象，不能为 NULL。
 * @param  now_tick 当前 FreeRTOS Tick。
 */
void FeedMotorControl_SetWait(FeedMotor_ControlTypeDef *control,
                              uint32_t now_tick);

/**
 * @brief  推进一次上弹/停止/下弹/停止状态机。
 * @param  control 任务独占的状态对象。
 * @param  config  测试时间和电流配置，时间字段为毫秒。
 * @param  feedback_online true 表示 C610 已收到新鲜反馈。
 * @param  now_tick 当前 FreeRTOS Tick；函数内部把毫秒配置转换为 Tick。
 * @retval true  本次调用发生了阶段切换或首次进入上弹。
 * @retval false 仍处于当前阶段，或因反馈无效保持等待。
 */
bool FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
                             const FeedMotor_CommandConfigTypeDef *config,
                             bool feedback_online,
                             uint32_t now_tick);

/**
 * @brief  读取当前阶段应发送的目标电流。
 * @param  control 当前状态对象。
 * @param  config  测试电流配置，单位为 C610 原始值。
 * @retval 当前阶段电流；等待和停止阶段返回 0。
 */
int16_t FeedMotorControl_GetCurrent(
    const FeedMotor_ControlTypeDef *control,
    const FeedMotor_CommandConfigTypeDef *config);

/**
 * @brief  获取阶段的静态日志名称。
 * @param  phase 状态机阶段。
 * @retval 静态字符串，不需要释放。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase);

#endif /* TASK_FEED_MOTOR_CONTROL_H */
