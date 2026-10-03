/**
  ******************************************************************************
  * @file    task_feed_motor.h
  * @brief   M2006 供弹电机任务的入口声明。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_H
#define TASK_FEED_MOTOR_H /* 防止供弹任务入口声明被重复包含。 */

/**
 * @brief  唯一 M2006 供弹电机的 FreeRTOS 任务入口。
 * @param  argument CubeMX 传入的任务参数，当前未使用。
 * @retval None 任务初始化成功后永不返回；初始化失败时进入安全等待。
 * @note   只能由任务上下文执行。CAN 接收在 ISR 中完成，串口诊断在任务中限频执行。
 */
void task_feed_motor_entry(void *argument);

#endif /* TASK_FEED_MOTOR_H */
