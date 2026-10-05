/**
  ******************************************************************************
  * @file    task_feed_motor_command.h
  * @brief   供弹电机自循环测试的静态配置接口。
  *
 * 当前没有接入遥控器协议，因此命令层保存供弹方向、阶段时间和电流的
 * 默认调参值。硬件测试头文件和正式命令默认值共享同一组参数，调参后
 * 不会因为两处常量不同而产生行为偏差。所有时间字段均为毫秒，电流字段
 * 为 C610 原始值。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_COMMAND_H
#define TASK_FEED_MOTOR_COMMAND_H /* 防止配置接口被重复包含（避免结构体重复定义）。 */

#include <stdint.h>

typedef struct {
  uint8_t motor_id;                 /* C610 电调编号，1 对应反馈 ID 0x201（ID 决定聚合帧槽位）。 */
  uint32_t up_time_ms;              /* 上弹阶段持续时间，单位 ms（正方向保持多久）。 */
  uint32_t stop_time_ms;            /* 每次换向前的零电流缓冲时间，单位 ms（先卸力再反向）。 */
  uint32_t down_time_ms;            /* 下弹阶段持续时间，单位 ms（负方向保持多久）。 */
  uint32_t log_period_ms;           /* 周期诊断日志最小间隔，单位 ms（避免串口被周期输出占满）。 */
  int16_t up_current_raw;            /* 上弹方向目标电流原始值（C610 协议原始量）。 */
  int16_t down_current_raw;          /* 下弹方向目标电流原始值（通常为负值）。 */
} FeedMotor_CommandConfigTypeDef;

/**
 * @brief  获取唯一供弹电机的默认阶段和电流配置。
 * @param  config 输出配置对象，不能为 NULL；时间单位为毫秒，电流为 C610 原始值。
 * @note   函数只复制头文件中的调参值，不访问 CAN、FreeRTOS 或串口；正式命令接入后
 *         应继续复用这份配置，而不是重新定义一组电流范围。
 */
void FeedMotorCommand_GetDefault(FeedMotor_CommandConfigTypeDef *config);

#endif /* TASK_FEED_MOTOR_COMMAND_H（防止配置接口被重复包含） */
