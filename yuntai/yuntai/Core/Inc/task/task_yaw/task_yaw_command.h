/**
  ******************************************************************************
  * @file    task_yaw_command.h
  * @brief   Yaw 遥控器/键盘输入的最小抽象接口。
  *
  * DBUS 任务是唯一发布者；本模块提供一致快照，不能让多个任务/ISR 同时争抢写入。
  * 输入适配层只提交相对转速命令，Yaw 控制器负责把它转成目标角度；没有新命令或
  * 命令超时会自动禁用（正式运行时零电流并丢弃旧目标，不继续追踪旧方向）。
  * 通俗理解：命令像一张带时间戳的纸条，超过有效期就自动作废。
  ******************************************************************************
  */

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
 * @note   超时命令 enabled=false、velocity_permille=0，正式运行时必须零电流（有效回中则 enabled=true）。
 */
bool YawCommand_GetSnapshot(uint32_t now_ms, Yaw_CommandTypeDef *command);

#endif /* TASK_YAW_COMMAND_H（防止 Yaw 命令接口被重复包含） */
