/**
 * @file task_pitch_command.h
 * @brief Pitch命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 短任务临界区复制全部字段，避免许可、按钮或速度来自不同帧。ISR 不调用接口。
 * 过期命令停止目标移动；正式 GimbalAxis 按零速度保持位置。
 */
/* 调用链：DBUS 接收快照→Submit 值复制→所属任务 GetSnapshot→检查许可→运行时输出。
 * 发布失败不修改旧邮箱；读取返回成功仅代表复制完成，不代表数据仍在线。禁止 ISR 发布或读取。 */
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
 * @brief 获取一致命令并处理过期（过期 enabled=false，正式运行时按零速度保持）。
 * @param now_ms HAL 当前 ms，与接收时间同源。
 * @param command 输出副本；禁用时速度为零，回中有效命令则 enabled=true。
 * @retval true 已复制；false 空指针，输出不变。
 * @note 仅任务上下文；读取成功不代表命令有效，必须同时检查 enabled。
 */
bool PitchCommand_GetSnapshot(uint32_t now_ms, Pitch_CommandTypeDef *command);
#endif /* TASK_PITCH_COMMAND_H */
