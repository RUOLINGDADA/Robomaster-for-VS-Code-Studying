/**
  ******************************************************************************
  * @file    task_feed_motor_runtime.h
  * @brief   组合 C610 驱动、正式供弹控制占位、保护和诊断日志。
  *
  * 运行时对象由唯一的 task_feed_motor 任务独占。HAL 毫秒时间用于 C610
  * 反馈超时，FreeRTOS Tick 用于阶段计时和日志限频，两个时间基准不能
  * 直接相减。CAN 接收仍由统一 FIFO0 ISR 分发，运行时只在任务上下文发帧
  * 和打印串口。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_RUNTIME_H
#define TASK_FEED_MOTOR_RUNTIME_H /* 防止运行时接口重复包含。 */

#include "bsp/c610_m2006/c610_m2006.h"
#include "task/task_feed_motor/task_feed_motor_command.h"
#include "task/task_feed_motor/task_feed_motor_control.h"
#include "task/task_feed_motor/task_feed_motor_protection.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  C610_M2006_HandleTypeDef motor;
  FeedMotor_CommandConfigTypeDef command;
  FeedMotor_ControlTypeDef control;
  FeedMotor_ProtectionOutputTypeDef protection;
  uint32_t last_log_tick;
  bool initialized;
} FeedMotor_RuntimeTypeDef;

/**
 * @brief  初始化唯一供弹电机句柄和测试运行时。
 * @param  runtime 任务独占的运行时对象。
 * @retval true 初始化成功；false 注册失败，输出保持关闭。
 */
bool FeedMotor_RuntimeInit(FeedMotor_RuntimeTypeDef *runtime);

/**
 * @brief  执行一个正式供弹控制周期。
 * @param  runtime 任务独占的运行时对象。
 * @param  now_ms HAL_GetTick() 返回的毫秒时间，用于反馈超时。
 * @param  now_tick FreeRTOS Tick，用于阶段计时和日志限频。
 * @note   只能在任务上下文调用，会发送 C610 控制帧和串口日志。当前没有
 *         真实供弹命令适配层时保持零输出；上弹/下弹自循环属于驱动同级
 *         的 bsp/c610_m2006 硬件调参文件。
 */
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime,
                               uint32_t now_ms,
                               uint32_t now_tick);

#endif /* TASK_FEED_MOTOR_RUNTIME_H */
