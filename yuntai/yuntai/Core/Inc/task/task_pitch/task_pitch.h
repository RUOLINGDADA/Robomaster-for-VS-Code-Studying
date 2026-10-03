/**
  ******************************************************************************
  * @file    task_pitch.h
  * @brief   发射云台 Pitch 任务入口声明。
  *
  * CubeMX 创建任务并调用此强入口；独立配置放 task_pitch_config.h，
  * 正式控制与硬件调参共用 app/gimbal。入口不在 ISR 中调用。
  ******************************************************************************
  */

#ifndef TASK_PITCH_H
#define TASK_PITCH_H /* 防止 Pitch 任务入口声明被重复包含。 */

/**
 * @brief  执行 Pitch 标定、固定角度调参或正式零输出任务。
 * @param  argument CubeMX/FreeRTOS 参数，当前未使用。
 * @retval None 任务入口不会返回；每 2 ms 唤醒一次。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用。
 */
void task_pitch_entry(void *argument);

#endif /* TASK_PITCH_H */
