/**
 * @file task_chassis.h
 * @brief CAN1 四轮底盘的唯一 FreeRTOS 入口。
 *
 * CubeMX 手动新增 task_chassis：Normal2，1024 words，As weak。
 * 本模块提供强入口，只初始化和按 2 ms 调度，不手写任务创建或重复启动 DBUS。
 */
#ifndef TASK_CHASSIS_H
#define TASK_CHASSIS_H /* 防止重复包含。 */
/**
 * @brief 初始化底盘并固定周期调度，任务永不返回。
 * @param argument CubeMX 参数，未使用，填 NULL。
 * @retval None；失败让出 CPU，每 100 ms 重试。
 * @note 仅由 CubeMX 创建的新任务进入；不从 main 或其它任务直接调用。
 */
void task_chassis_entry(void *argument);
#endif /* TASK_CHASSIS_H */
