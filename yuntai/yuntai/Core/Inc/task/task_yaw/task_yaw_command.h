/**
 * @file task_yaw_command.h
 * @brief Yaw命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 单发布者序列号和内存屏障保证一致复制；volatile 本身不保证多字段原子性。
 * 最多重试 4 次；失败返回禁用命令，不能在 ISR 忙等发布者完成。
 * 过期命令停止目标移动；正式 GimbalAxis 按零速度保持位置。
 */

/* 调用链：DBUS 接收快照→Submit 值复制→所属任务 GetSnapshot→检查许可→运行时输出。
 * 发布失败不修改旧邮箱；读取返回成功仅代表复制完成，不代表数据仍在线。禁止 ISR 发布或读取。 */
#ifndef TASK_YAW_COMMAND_H
#define TASK_YAW_COMMAND_H /* 防止 Yaw 命令接口被重复包含（避免同一份声明出现两次）。 */

#include <stdbool.h>
#include <stdint.h>
#include "app/gimbal/gimbal_command.h"
#include "task/task_yaw/task_yaw_config.h"

typedef Gimbal_CommandTypeDef Yaw_CommandTypeDef; /* Yaw 任务对通用命令的兼容别名（两者字段完全相同）。 */

/**
 * @brief  提交一份新的 Yaw 命令快照（发布者只交出一份值，不交出指针）。
 * @param  command 命令内容；函数只复制结构体，不保存调用者指针（局部变量也可以传入）。
 * @retval true 参数有效并已发布；false 参数为空或速度越界（失败不会更新旧命令）。
 * @note   当前固定由 DBUS 任务单独发布；不阻塞、不打印（序号协议不支持多发布者交错）。
 */
bool YawCommand_Submit(const Yaw_CommandTypeDef *command);

/**
 * @brief  获取命令并执行超时保护（读取到的是一份完整命令）。
 * @param  now_ms 与提交时间戳相同来源的毫秒时基（不能混用 FreeRTOS Tick）。
 * @param  command 输出命令快照（失败时函数会写入安全的中立命令）。
 * @retval true 读取到一致快照；false 参数为空或发布者正在更新（调用者应保持安全状态）。
 * @note   超时命令 enabled=false、velocity_permille=0.0f，正式运行时按零速度保持位置（有效回中则 enabled=true）。
 */
bool YawCommand_GetSnapshot(uint32_t now_ms, Yaw_CommandTypeDef *command);

#endif /* TASK_YAW_COMMAND_H（防止 Yaw 命令接口被重复包含） */
