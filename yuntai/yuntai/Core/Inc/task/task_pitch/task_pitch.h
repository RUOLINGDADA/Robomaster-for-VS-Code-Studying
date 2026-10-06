/**
 * @file task_pitch.h
 * @brief Pitch FreeRTOS 入口与周期调度。
 *
 * CAN1 反馈 0x206 的独立 GM6020。
 * CubeMX/.ioc 创建该任务：Normal2，栈 2048 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * Normal2 与另一轴及供弹同级；输入由更高优先级 DBUS 任务发布，反馈由 CAN ISR 更新。
 * main 先配置并启动 CAN；任务初始化只注册独立轴，首帧到达后才建立控制历史。
 * 注册失败后每 1000 ms 尝试输出诊断并等待，不重试注册、不进入控制循环；须修正配置后复位。
 * 正式、只读标定、固定目标测试在编译时选一条路径，不能在同周期分别写电流。
 */

/* 链接入口：freertos.c 的 osThreadNew 调用这里的强定义，CubeMX 的同名 __weak 函数只作兜底。
 * 顶层 CMake 必须链接本任务源文件；不要从 main/ISR 直接调用永不返回的任务入口。 */
#ifndef TASK_PITCH_H
#define TASK_PITCH_H /* 防止 Pitch 任务入口声明被重复包含（避免同一函数声明出现两次）。 */

/**
 * @brief  执行 Pitch 标定、固定角度调参或正式输入控制任务（编译期宏选择模式）。
 * @param  argument CubeMX/FreeRTOS 参数，当前未使用（保持入口签名不变）。
 * @retval None 任务入口不会返回；每 2 ms 唤醒一次（用绝对唤醒避免周期漂移）。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用（避免并发控制）。
 */
void task_pitch_entry(void *argument);

#endif /* TASK_PITCH_H */
