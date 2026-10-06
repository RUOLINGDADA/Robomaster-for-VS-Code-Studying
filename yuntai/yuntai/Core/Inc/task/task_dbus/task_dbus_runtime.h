/**
 * @file task_dbus_runtime.h
 * @brief DBUS 摇杆、鼠标和左键的任务适配。
 *
 * PC11/USART3 的 18 字节 DBUS 协议。
 * 仅 DBUS 任务调用。CH0→Yaw、CH1→Pitch；鼠标 X/Y 由独立模块累计为虚拟速度。
 * 合成速度限制为 ±1000‰，命令保留浮点小数。左键按住发布连续发射意图。
 * 三个命令均使用接收帧 HAL ms；重复发布不续期，离线使全部命令失效。
 * 本模块只发布命令和限频日志，不写 CAN 电流或 PWM。
 */
/* 调用顺序：USART3/DMA 初始化→Dbus_Init→DBUS 任务周期调用 RunCycle。
 * 恢复等待留在任务，发布命令保留原帧时间；UART 日志繁忙不应延长输入有效期。 */
#ifndef TASK_DBUS_RUNTIME_H
#define TASK_DBUS_RUNTIME_H /* 防止 DBUS 周期接口重复包含。 */
#include <stdint.h>
/**
 * @brief 处理当前 DBUS 快照并向两轴和供弹发布同源命令（不靠转发旧帧刷新有效期）。
 * @param now_ms HAL_GetTick 的当前 ms，不能传 FreeRTOS Tick。
 * @retval None 更新命令邮箱与日志时刻。
 * @note 仅 DBUS 任务调用，作为两轴与供弹命令的唯一发布者；UART 忙时不等待。
 */
void DbusTask_RuntimeRunCycle(uint32_t now_ms);
#endif /* TASK_DBUS_RUNTIME_H */
