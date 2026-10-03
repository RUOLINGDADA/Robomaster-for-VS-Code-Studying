/**
  ******************************************************************************
  * @file    task_yaw_command.h
  * @brief   Yaw 遥控器/键盘输入的最小抽象接口。
  *
  * 当前工程还没有绑定具体遥控器协议，因此本模块先提供线程/中断都能
  * 使用的快照接口。输入适配层只提交相对转速命令，Yaw 控制器负责把它
  * 转成目标角度；没有新命令或命令超时会自动回到中立状态。
  ******************************************************************************
  */

#ifndef TASK_YAW_COMMAND_H
#define TASK_YAW_COMMAND_H /* 防止 Yaw 命令接口被重复包含。 */

#include <stdbool.h>
#include <stdint.h>
#include "app/gimbal/gimbal_command.h"

typedef Gimbal_CommandTypeDef Yaw_CommandTypeDef; /* Yaw 任务对通用命令的兼容别名。 */

#define YAW_COMMAND_TIMEOUT_MS 100U /* 命令超过 100 ms 未刷新即自动中立。 */

/**
 * @brief  提交一份新的 Yaw 命令快照。
 * @param  command 命令内容；函数只复制结构体，不保存调用者指针。
 * @retval true 参数有效并已发布；false 参数为空或速度越界。
 * @note   可在普通任务或短小的输入 ISR 中调用；不阻塞、不打印。
 */
bool YawCommand_Submit(const Yaw_CommandTypeDef *command);

/**
 * @brief  获取命令并执行超时保护。
 * @param  now_ms 与提交时间戳相同来源的毫秒时基。
 * @param  command 输出命令快照。
 * @retval true 读取到一致快照；false 参数为空或发布者正在更新。
 * @note   超时命令会被改为 enabled=false、velocity_permille=0。
 */
bool YawCommand_GetSnapshot(uint32_t now_ms, Yaw_CommandTypeDef *command);

#endif /* TASK_YAW_COMMAND_H */
