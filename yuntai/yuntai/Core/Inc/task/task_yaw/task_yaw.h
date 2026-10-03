/**
  ******************************************************************************
  * @file    task_yaw.h
  * @brief   Yaw FreeRTOS 任务入口声明。
  *
  * CubeMX 在 freertos.c 中创建的入口名称必须保持 task_yaw_entry；具体的
  * 初始化和命令适配放同目录，控制、保护与诊断共用 app/gimbal，避免把
  * 业务逻辑重新堆回自动生成文件。
  ******************************************************************************
  */

#ifndef TASK_YAW_H
#define TASK_YAW_H /* 防止 Yaw 任务入口声明被重复包含。 */

/**
 * @brief  执行 Yaw 水平轴控制任务。
 * @param  argument CubeMX/FreeRTOS 任务参数，当前未使用。
 * @retval None 任务入口不会返回。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用。
 */
void task_yaw_entry(void *argument);

#endif /* TASK_YAW_H */
