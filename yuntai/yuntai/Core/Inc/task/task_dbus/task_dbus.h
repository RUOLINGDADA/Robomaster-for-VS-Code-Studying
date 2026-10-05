/**
 * @file task_dbus.h
 * @brief DBUS 输入任务入口（只负责初始化和周期调度，电机控制留在各轴任务）。
 */
#ifndef TASK_DBUS_H
#define TASK_DBUS_H /* 防止 DBUS 任务入口重复声明（不共享轴的控制状态）。 */
/**
 * @brief 创建接收资源，每 2 ms 调度 DBUS 处理与输入适配。
 * @param argument 未使用的 FreeRTOS 任务参数。
 * @retval None 任务永不返回；初始化失败时限频重试，双轴仍收不到有效命令。
 * @note 由 CubeMX 创建，优先级 Normal3，栈 384 words；ISR 不调用此入口。
 */
void task_dbus_entry(void *argument);
#endif /* TASK_DBUS_H */
