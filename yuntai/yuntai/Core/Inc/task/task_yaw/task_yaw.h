/**
  ******************************************************************************
  * @file    task_yaw.h
  * @brief   Yaw FreeRTOS 任务入口声明（负责每 2 ms 调度一个水平轴运行周期）。
  *
  * CubeMX 在 freertos.c 中创建的入口名称必须保持 task_yaw_entry；具体的初始化
  * 和命令适配放同目录，控制、保护与诊断共用 app/gimbal（任务只负责调度，不重复算法）。
  ******************************************************************************
  */

#ifndef TASK_YAW_H
#define TASK_YAW_H /* 防止 Yaw 任务入口声明被重复包含（避免同一函数声明出现两次）。 */

/**
 * @brief  执行 Yaw 水平轴控制任务（初始化后按固定周期选择测试或正式模式）。
 * @param  argument CubeMX/FreeRTOS 任务参数，当前未使用（保持入口签名不变）。
 * @retval None 任务入口不会返回（结束条件由系统复位或调度器停止决定）。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用（避免并发写控制状态）。
 */
void task_yaw_entry(void *argument);

#endif /* TASK_YAW_H */
