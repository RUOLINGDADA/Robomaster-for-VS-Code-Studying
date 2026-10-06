/**
 * @file task_feed_motor.h
 * @brief 供弹 FreeRTOS 入口与周期调度。
 *
 * CAN1 ID1 M2006 与 PE9/PE11 双 C615。
 * CubeMX/.ioc 创建该任务：Normal2，栈 1024 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * 正式发射与显式硬件测试互斥。部分初始化失败保留停止输出，并限频重试。
 * Normal2 与两个云台任务同级；100 ms 命令/反馈安全门由本任务自行检查。
 * main 先完成 CAN1/TIM1；本任务注册 M2006、写停止值并启动双 PWM，不重复创建任务。
 * 初始化重试保留已注册句柄；HAL 失败不证明硬件已停，软件只能提交停止命令。
 */

/* 链接入口：freertos.c 的 osThreadNew 调用这里的强定义，CubeMX 的同名 __weak 函数只作兜底。
 * 顶层 CMake 必须链接本任务源文件；不要从 main/ISR 直接调用永不返回的任务入口。 */
#ifndef TASK_FEED_MOTOR_H
#define TASK_FEED_MOTOR_H /* 防止供弹任务入口声明被重复包含（避免同一声明出现两次）。 */

/**
 * @brief  唯一 M2006 供弹电机的 FreeRTOS 任务入口（按固定周期推进控制）。
 * @param  argument CubeMX 传入的任务参数，当前未使用。
 * @retval None 任务初始化成功后永不返回；部分初始化失败时保留停止值，并每 100 ms 重试。
 * @note   只能由任务上下文执行。CAN 接收在 ISR 中完成，串口诊断在任务中限频执行。
 */
void task_feed_motor_entry(void *argument);

#endif /* TASK_FEED_MOTOR_H（防止供弹任务入口声明被重复包含） */
