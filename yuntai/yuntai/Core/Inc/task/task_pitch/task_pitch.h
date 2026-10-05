/**
  ******************************************************************************
  * @file    task_pitch.h
  * @brief   发射云台 Pitch 任务入口声明（负责每 2 ms 调度一个俯仰轴周期）。
  *
  * CubeMX 创建任务并调用此入口；独立配置放 task_pitch_config.h，正式控制与硬件
  * 调参共用 app/gimbal（测试和正式模式使用同一份轴状态）。入口不在 ISR 中调用。
  ******************************************************************************
  */

#ifndef TASK_PITCH_H
#define TASK_PITCH_H /* 防止 Pitch 任务入口声明被重复包含（避免同一函数声明出现两次）。 */

/**
 * @brief  执行 Pitch 标定、固定角度调参或正式零输出任务（编译期宏选择模式）。
 * @param  argument CubeMX/FreeRTOS 参数，当前未使用（保持入口签名不变）。
 * @retval None 任务入口不会返回；每 2 ms 唤醒一次（用绝对唤醒避免周期漂移）。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用（避免并发控制）。
 */
void task_pitch_entry(void *argument);

#endif /* TASK_PITCH_H */
