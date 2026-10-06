/**
 * @file gimbal_command.h
 * @brief Yaw/Pitch 共用的相对速度命令结构。
 *
 * 输入层提交 ±1000‰ 速度和接收帧的 HAL ms 时间戳。
 * 命令邮箱在任务间复制；本文件只定义数据，不保护共享内存。
 * 命令失效停止目标移动；正式轴仍保持位置。反馈失效才清零电流。
 */
#ifndef GIMBAL_COMMAND_H
#define GIMBAL_COMMAND_H /* 防止通用云台命令结构重复包含（避免结构体定义重复）。 */

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  float velocity_permille; /* -1000.0~1000.0‰；保留鼠标小数输入，负/正表示相反方向。 */
  bool enabled; /* true 表示输入有效；false 停止目标移动，正式轴仍保持位置。反馈失效才清零。 */
  bool fixed_target; /* true 表示固定目标测试意图；零速度不应被边界过滤当成遥控器回中。 */
  uint32_t timestamp_ms; /* 原始接收帧的 HAL ms 时间戳。重复发布不能刷新有效期。 */
} Gimbal_CommandTypeDef;

#endif /* GIMBAL_COMMAND_H（防止命令结构被重复包含） */
