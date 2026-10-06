/**
 * @file task_dbus.h
 * @brief DBUS 输入 FreeRTOS 入口与周期调度。
 *
 * PC11/USART3 的 18 字节 DBUS 协议。
 * CubeMX/.ioc 创建该任务：Normal3，栈 2048 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * Normal3 使输入任务先于 Normal2 电机任务运行；该顺序不替代各任务的超时检查。
 * 先由 main 完成 USART3/DMA 初始化，再启动接收；尚未同步或无合法帧时不发布有效输入。
 * 初始化失败每 100 ms 重试；接收故障的后续恢复由 Dbus_Process 在任务中处理。
 */
/* 链接入口：freertos.c 的 osThreadNew 调用这里的强定义，CubeMX 的同名 __weak 函数只作兜底。
 * 顶层 CMake 必须链接本任务源文件；不要从 main/ISR 直接调用永不返回的任务入口。 */
#ifndef TASK_DBUS_H
#define TASK_DBUS_H /* 防止 DBUS 任务入口重复声明（不共享轴的控制状态）。 */
/**
 * @brief 启动接收，每 2 ms 调度 DBUS 解码与三路命令发布。
 * @param argument 未使用的 FreeRTOS 任务参数。
 * @retval None 任务永不返回；初始化失败时限频重试，云台与供弹均收不到有效命令。
 * @note 由 CubeMX 创建，优先级 Normal3，栈 2048 words（来源 yuntai.ioc）；ISR 不调用此入口。
 */
void task_dbus_entry(void *argument);
#endif /* TASK_DBUS_H */
