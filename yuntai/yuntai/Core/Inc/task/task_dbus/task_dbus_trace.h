/**
 * @file task_dbus_trace.h
 * @brief DBUS 原始帧与控制命令的有界采集接口。
 *
 * 只由 DBUS 任务调用；原始帧逐份复制，命令使用已发布的同周期值。
 * 缓冲满时记录丢样，串口忙时保留待发记录，不阻塞或改变控制输入。
 */
#ifndef TASK_DBUS_TRACE_H
#define TASK_DBUS_TRACE_H /* 防止输入采集接口重复包含。 */

#include "bsp/dbus/dbus.h"

/**
 * @brief 复制一份已提交的原始合法帧。
 * @param frame 接收帧与 ISR 时间戳；函数返回后不持有指针。
 * @retval None。关闭采集或缓冲满时不影响接收。
 * @note 仅 DBUS 任务，不能在 ISR 中调用。
 */
void DbusTrace_RecordFrame(const Dbus_ReceivedFrameTypeDef *frame);

/**
 * @brief 记录新帧、命令变化或周期心跳。
 * @param input 本周期唯一 DBUS 快照；不再次读取驱动或消费鼠标增量。
 * @param control_ms 算法本周期使用的 HAL ms。
 * @param virtual_x 鼠标 X 输出，‰。
 * @param virtual_y 鼠标 Y 输出，‰。
 * @param yaw 已发布的合成 Yaw 命令，‰。
 * @param pitch 已发布的合成 Pitch 命令，‰。
 * @param enabled 已发布的两轴命令许可。
 * @param x_state 鼠标 X 阶段，0空闲、1保持、2回中、3新输入。
 * @param y_state 鼠标 Y 阶段，编码与 X 相同。
 * @retval None。日志量化结果不回写算法或命令。
 * @note 仅 DBUS 任务；重复调用不刷新控制输入有效期。
 */
void DbusTrace_RecordCommand(const Dbus_SnapshotTypeDef *input, uint32_t control_ms,
    float virtual_x, float virtual_y, float yaw, float pitch, bool enabled,
    uint8_t x_state, uint8_t y_state);

/**
 * @brief 非阻塞提交字段说明、接收统计或一批采集记录。
 * @param input 本周期 DBUS 快照；统计不混读 ISR 字段。
 * @param now_ms 本周期 HAL ms，用于统计周期。
 * @retval None。DMA 拒绝时记录继续保留，接受后才出队。
 * @note 仅 DBUS 任务；复用唯一 Log_TryPrintf 发送后端。
 */
void DbusTrace_Flush(const Dbus_SnapshotTypeDef *input, uint32_t now_ms);

#endif /* TASK_DBUS_TRACE_H */
