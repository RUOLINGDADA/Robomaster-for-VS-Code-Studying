/**
  ******************************************************************************
  * @file    test_c610_m2006_self_cycle.h
  * @brief   唯一 M2006 供弹电机上弹/停止/下弹/停止硬件调参接口。
  *
  * 测试参数用于确定供弹方向、电流和阶段时间。函数是非阻塞单步状态机，
  * 内部复用 C610/M2006 驱动的反馈超时、电流钳位和聚合发送接口。调参完成
  * 后把宏设为 0，任务将切换到正式供弹运行时。
  ******************************************************************************
  */

#ifndef TEST_C610_M2006_SELF_CYCLE_H
#define TEST_C610_M2006_SELF_CYCLE_H /* 防止供弹硬件测试接口重复包含。 */

#include "bsp/c610_m2006/c610_m2006.h"

#include <stdint.h>

#define C610_M2006_HARDWARE_TEST_ENABLE 1 /* 1=烧录上板自循环，0=进入正式任务。 */
#define C610_M2006_TEST_UP_TIME_MS 500U /* 上弹阶段持续时间，单位 ms。 */
#define C610_M2006_TEST_STOP_TIME_MS 500U /* 换向前零电流阶段持续时间，单位 ms。 */
#define C610_M2006_TEST_DOWN_TIME_MS 500U /* 下弹阶段持续时间，单位 ms。 */
#define C610_M2006_TEST_UP_CURRENT_RAW 700 /* 上弹目标电流，C610 原始值。 */
#define C610_M2006_TEST_DOWN_CURRENT_RAW (-500) /* 下弹目标电流，C610 原始值。 */
#define C610_M2006_TEST_LOG_PERIOD_MS 500U /* 自循环诊断日志最短间隔，单位 ms。 */

/**
 * @brief  执行一次供弹电机硬件自循环。
 * @param  motor 已初始化的唯一 C610/M2006 电机句柄。
 * @param  now_ms HAL_GetTick() 返回的当前毫秒时间。
 * @note   函数内部保存阶段状态，不需要额外初始化；只能在任务上下文调用，
 *         每次调用只推进一个阶段，不得在 CAN ISR 中调用或等待。
 */
void C610_M2006_TestSelfCycle_Run(C610_M2006_HandleTypeDef *motor,
                                  uint32_t now_ms);

#endif /* TEST_C610_M2006_SELF_CYCLE_H */
