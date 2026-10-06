/**
 * @file task_feed_motor_command.h
 * @brief 供弹命令邮箱与 100 ms 有效期。
 *
 * DBUS 任务是唯一发布者；所属电机任务读取值副本，不保存外部指针。
 * 接收帧时间戳使用 HAL ms。超时使用同源无符号差值，不靠 0 ms 判定无效。
 * 本模块不访问 DMA，不写电机，不创建 RTOS 对象。
 * 短任务临界区复制全部字段，避免许可、按钮或速度来自不同帧。ISR 不调用接口。
 */
/* 调用链：DBUS 接收快照→Submit 值复制→所属任务 GetSnapshot→检查许可→运行时输出。
 * 发布失败不修改旧邮箱；读取返回成功仅代表复制完成，不代表数据仍在线。禁止 ISR 发布或读取。 */
#ifndef TASK_FEED_MOTOR_COMMAND_H
#define TASK_FEED_MOTOR_COMMAND_H /* 防止重复包含。 */
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool enabled; /* 当前 DBUS 输入有效；false 强制故障停机。 */
  bool fire_requested; /* 最新合法帧的左键状态；true 请求连续发射。 */
  uint32_t timestamp_ms; /* DMA 完整帧接收时刻，HAL ms；转发不能刷新此值。 */
} FeedMotor_CommandTypeDef;

/**
 * @brief 发布一份完整发射命令。
 * @param command 帧时间戳与按钮值；不保留外部指针。
 * @retval true 已发布；false 空指针，旧命令不变。
 * @note 仅 DBUS 任务调用；不创建队列，不阻塞。
 */
bool FeedMotorCommand_Submit(const FeedMotor_CommandTypeDef *command);
/**
 * @brief 读取命令并执行 100 ms 超时归零。
 * @param now_ms HAL_GetTick 时间；不能传 FreeRTOS Tick。
 * @param command 输出快照；无命令或过期时两个布尔值为 false。
 * @retval true 有过命令；false 空指针或尚未发布。
 * @note 仅任务调用；短临界区只复制，不等待新帧。
 */
bool FeedMotorCommand_GetSnapshot(uint32_t now_ms, FeedMotor_CommandTypeDef *command);
#endif /* TASK_FEED_MOTOR_COMMAND_H */
