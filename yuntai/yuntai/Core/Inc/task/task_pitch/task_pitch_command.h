/**
 * @file task_pitch_command.h
 * @brief Pitch 独立命令快照与超时（接口字段与 Yaw 一样，邮箱和控制历史各自独立）。
 */
#ifndef TASK_PITCH_COMMAND_H
#define TASK_PITCH_COMMAND_H /* 防止 Pitch 命令类型重复定义。 */
#include "app/gimbal/gimbal_command.h"
#include "task/task_pitch/task_pitch_config.h"

typedef Gimbal_CommandTypeDef Pitch_CommandTypeDef; /* -1000~1000‰ 速度、使能、HAL ms 时间戳（不是角度目标）。 */
/**
 * @brief 发布 Pitch 相对速度命令副本（不保留外部指针）。
 * @param command 速度须在 ±1000‰，timestamp_ms 来自接收帧 HAL 时刻。
 * @retval true 已发布；false 参数非法，旧命令不变。
 * @note 发布者固定为 DBUS 任务；短临界区防止读取半份命令，不能用于多个模块争抢控制权。
 */
bool PitchCommand_Submit(const Pitch_CommandTypeDef *command);
/**
 * @brief 获取一致命令并处理过期（过期 enabled=false，正式运行时必须零输出）。
 * @param now_ms HAL 当前 ms，与接收时间同源。
 * @param command 输出副本；禁用时速度为零，回中有效命令则 enabled=true。
 * @retval true 已复制；false 空指针，输出不变。
 * @note 仅任务上下文；读取成功不代表命令有效，必须同时检查 enabled。
 */
bool PitchCommand_GetSnapshot(uint32_t now_ms, Pitch_CommandTypeDef *command);
#endif /* TASK_PITCH_COMMAND_H */
