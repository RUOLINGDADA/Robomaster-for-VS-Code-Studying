/**
  ******************************************************************************
  * @file    task_feed_motor_command.c
  * @brief   供弹自循环测试参数实现。
  ******************************************************************************
  */

#include "task/task_feed_motor/task_feed_motor_command.h"
#include "bsp/c610_m2006/test_c610_m2006_self_cycle.h"

#include <stddef.h>

#define FEED_MOTOR_DEFAULT_ID 1U /* CAN 反馈 ID 为 0x200 + 1 = 0x201。 */
#define FEED_MOTOR_DEFAULT_UP_TIME_MS C610_M2006_TEST_UP_TIME_MS /* 正式命令默认复用上弹调参值，单位 ms。 */
#define FEED_MOTOR_DEFAULT_STOP_TIME_MS C610_M2006_TEST_STOP_TIME_MS /* 正式命令默认复用停止调参值，单位 ms。 */
#define FEED_MOTOR_DEFAULT_DOWN_TIME_MS C610_M2006_TEST_DOWN_TIME_MS /* 正式命令默认复用下弹调参值，单位 ms。 */
#define FEED_MOTOR_DEFAULT_LOG_PERIOD_MS C610_M2006_TEST_LOG_PERIOD_MS /* 正式日志默认复用调参日志间隔，单位 ms。 */
#define FEED_MOTOR_DEFAULT_UP_CURRENT_RAW C610_M2006_TEST_UP_CURRENT_RAW /* 正式命令默认复用上弹电流原始值。 */
#define FEED_MOTOR_DEFAULT_DOWN_CURRENT_RAW C610_M2006_TEST_DOWN_CURRENT_RAW /* 正式命令默认复用下弹电流原始值。 */

void FeedMotorCommand_GetDefault(FeedMotor_CommandConfigTypeDef *config) {
  if (config == NULL) {
    return;
  }

  *config = (FeedMotor_CommandConfigTypeDef){
      .motor_id = FEED_MOTOR_DEFAULT_ID,
      .up_time_ms = FEED_MOTOR_DEFAULT_UP_TIME_MS,
      .stop_time_ms = FEED_MOTOR_DEFAULT_STOP_TIME_MS,
      .down_time_ms = FEED_MOTOR_DEFAULT_DOWN_TIME_MS,
      .log_period_ms = FEED_MOTOR_DEFAULT_LOG_PERIOD_MS,
      .up_current_raw = FEED_MOTOR_DEFAULT_UP_CURRENT_RAW,
      .down_current_raw = FEED_MOTOR_DEFAULT_DOWN_CURRENT_RAW,
  };
}
