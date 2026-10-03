/**
  ******************************************************************************
  * @file    task_feed_motor_protection.h
  * @brief   供弹任务的通信失联和输出安全保护。
  *
  * 当前供弹功能只做有人值守的方向确认自循环，没有加入未经标定的堵转
  * 算法。保护模块仍单独存在，保证反馈失联时关闭输出，并为后续真实供弹
  * 状态机增加限位、堵转或急停条件时保持清晰的模块边界。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_PROTECTION_H
#define TASK_FEED_MOTOR_PROTECTION_H /* 防止保护接口重复包含。 */

#include <stdbool.h>

typedef struct {
  bool feedback_online;
} FeedMotor_ProtectionInputTypeDef;

typedef struct {
  bool allow_output;
  bool feedback_lost;
} FeedMotor_ProtectionOutputTypeDef;

/**
 * @brief  根据反馈在线状态计算供弹任务的最小安全输出许可。
 * @param  input  本周期输入；反馈离线时必须禁止输出。
 * @param  output 输出保护结果，不能为 NULL。
 * @note   函数不做堵转判断，也不访问硬件；复杂保护应继续放在本模块，
 *         由运行时统一执行零电流和状态回退。
 */
void FeedMotorProtection_Update(
    const FeedMotor_ProtectionInputTypeDef *input,
    FeedMotor_ProtectionOutputTypeDef *output);

#endif /* TASK_FEED_MOTOR_PROTECTION_H */
