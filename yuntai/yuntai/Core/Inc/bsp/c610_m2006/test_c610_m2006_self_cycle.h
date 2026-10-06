/**
 * @file test_c610_m2006_self_cycle.h
 * @brief C610/M2006 非阻塞往返硬件测试接口。
 *
 * 参数由供弹任务提供。驱动不读取任务配置。只在任务上下文调用。
 * 上板前确认可急停。反馈过期时接口输出零电流。
 */
#ifndef TEST_C610_M2006_SELF_CYCLE_H
#define TEST_C610_M2006_SELF_CYCLE_H /* 防止测试接口重复包含。 */

#include "bsp/c610_m2006/c610_m2006.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool enabled; /* 是否运行自循环；false 时只输出零电流。 */
  uint32_t up_time_ms; /* 正向阶段时长，1~INT32_MAX ms。 */
  uint32_t stop_time_ms; /* 换向停顿时长，1~INT32_MAX ms。 */
  uint32_t down_time_ms; /* 反向阶段时长，1~INT32_MAX ms。 */
  int16_t up_current_raw; /* 正向目标电流，C610 raw，-10000~10000。 */
  int16_t down_current_raw; /* 反向目标电流，C610 raw，-10000~10000。 */
  uint32_t log_period_ms; /* 测试日志间隔，1~INT32_MAX ms。 */
} C610_M2006_TestConfigTypeDef;

/**
 * @brief 推进一次 C610/M2006 自循环。
 * @param motor 已初始化的 C610/M2006 句柄。
 * @param config 供弹任务提供的测试参数；调用期间保持有效。
 * @param now_ms HAL 毫秒时间。
 * @retval None。参数无效或反馈过期时输出零电流。
 * @note 唯一供弹任务独占内部阶段。函数不延时，不访问任务配置。
 */
void C610_M2006_TestSelfCycle_Run(C610_M2006_HandleTypeDef *motor,
                                  const C610_M2006_TestConfigTypeDef *config,
                                  uint32_t now_ms);

#endif /* TEST_C610_M2006_SELF_CYCLE_H */
