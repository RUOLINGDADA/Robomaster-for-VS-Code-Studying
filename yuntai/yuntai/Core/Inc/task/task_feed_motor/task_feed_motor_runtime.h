/**
  ******************************************************************************
  * @file    task_feed_motor_runtime.h
  * @brief   组合 C610 驱动、正式供弹控制占位和诊断日志。
  *
  * 运行时对象由唯一的 task_feed_motor 任务独占。HAL 毫秒时间用于 C610 反馈
  * 超时，FreeRTOS Tick 用于阶段计时和日志限频，两个时间基准不能直接相减。
  * CAN 接收仍由统一 FIFO0 ISR 分发，运行时只在任务上下文发帧和打印串口
  * （ISR 只取帧、解析和分发，不执行控制）。
  * 通俗理解：HAL 毫秒负责判断电机是否掉线，FreeRTOS Tick 负责任务内部计时，两种时间不能直接相减。
  ******************************************************************************
  */

#ifndef TASK_FEED_MOTOR_RUNTIME_H
#define TASK_FEED_MOTOR_RUNTIME_H /* 防止运行时接口重复包含（避免运行时结构重复定义）。 */

#include "bsp/c610_m2006/c610_m2006.h"
#include "task/task_feed_motor/task_feed_motor_command.h"
#include "task/task_feed_motor/task_feed_motor_control.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  C610_M2006_HandleTypeDef motor; /* 唯一供弹电机驱动句柄（反馈由 CAN ISR 更新）。 */
  FeedMotor_CommandConfigTypeDef command; /* 本次运行使用的时间和电流配置（单位已固定）。 */
  FeedMotor_ControlTypeDef control; /* 上弹/停止/下弹状态机（保存阶段和起点）。 */
  uint32_t last_log_tick; /* 上次日志的 FreeRTOS Tick（只用于限频）。 */
  bool initialized; /* true 表示驱动和运行时都已初始化（可以进入周期）。 */
} FeedMotor_RuntimeTypeDef;

/**
 * @brief  初始化唯一供弹电机句柄和测试运行时（默认关闭输出）。
 * @param  runtime 任务独占的运行时对象（保存电机和阶段状态）。
 * @retval true 初始化成功；false 注册失败，输出保持关闭（任务应继续发零）。
 */
bool FeedMotor_RuntimeInit(FeedMotor_RuntimeTypeDef *runtime);

/**
 * @brief  执行一个正式供弹控制周期（获取一次反馈并统一完成控制和发送）。
 * @param  runtime 任务独占的运行时对象（不能与其他任务并发调用）。
 * @param  now_ms HAL_GetTick() 返回的毫秒时间，用于反馈超时（与 ISR 时间戳同源）。
 * @param  now_tick FreeRTOS Tick，用于阶段计时和日志限频（不与 HAL ms 混算）。
 * @note   只能在任务上下文调用，会发送 C610 控制帧和串口日志。当前没有
 *         真实供弹命令适配层时保持零输出；上弹/下弹自循环属于驱动同级
 *         的 bsp/c610_m2006 硬件调参文件。
 */
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime,
                               uint32_t now_ms,
                               uint32_t now_tick);

#endif /* TASK_FEED_MOTOR_RUNTIME_H（防止运行时接口被重复包含） */
