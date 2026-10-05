/**
 * @file task_dbus_runtime.h
 * @brief DBUS 周期组合：接收处理、双轴命令发布和限频诊断（不发送电机电流）。
 */
#ifndef TASK_DBUS_RUNTIME_H
#define TASK_DBUS_RUNTIME_H /* 防止 DBUS 周期接口重复包含。 */
#include <stdint.h>
/**
 * @brief 处理最新帧并向两个轴发布同源命令（不靠转发旧帧刷新有效期）。
 * @param now_ms HAL_GetTick 的当前 ms，不能传 FreeRTOS Tick。
 * @retval None 更新命令邮箱与日志时刻。
 * @note 仅 DBUS 任务调用，作为两轴命令的唯一发布者；UART 忙时不等待。
 */
void DbusTask_RuntimeRunCycle(uint32_t now_ms);
#endif /* TASK_DBUS_RUNTIME_H */
