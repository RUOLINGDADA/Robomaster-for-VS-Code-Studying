/**
  ******************************************************************************
  * @file    test_gm6020_calibration.c
  * @brief   GM6020 双轴共用的只读机械角度标定实现。
  *
  * 通俗理解：标定期间始终发零电流，操作者用手移动云台，日志只负责把角度读出来。
  ******************************************************************************
  */

#include "bsp/gm6020/test_gm6020_calibration.h"

#include "usart.h"

#include <stddef.h>

#define GM6020_CALIBRATION_LOG_PERIOD_MS 200U /* 连续角度日志最短间隔，单位 HAL ms（避免串口刷屏）。 */
#define GM6020_CALIBRATION_PROMPT_PERIOD_MS 5000U /* 操作提示重发间隔，单位 HAL ms（提醒操作者记录边界）。 */

void Gm6020_TestCalibration_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    Gm6020_TestCalibrationTypeDef *test,
    uint32_t now_ms) {
  if (motor == NULL || calibration == NULL || test == NULL) {
    return;
  }
  if (test->axis_name == NULL) {
    test->axis_name = "云台轴";
  }
  if (test->first_limit_name == NULL) {
    test->first_limit_name = "第一侧限位";
  }
  if (test->second_limit_name == NULL) {
    test->second_limit_name = "第二侧限位";
  }
  const uint32_t log_period_ms = test->log_period_ms != 0U
      ? test->log_period_ms : GM6020_CALIBRATION_LOG_PERIOD_MS;
  const uint32_t prompt_period_ms = test->prompt_period_ms != 0U
      ? test->prompt_period_ms : GM6020_CALIBRATION_PROMPT_PERIOD_MS;
  (void)Gm6020_Process(motor, now_ms);
  Gm6020_SnapshotTypeDef snapshot = {0};
  const bool snapshot_valid = Gm6020_GetSnapshot(motor, &snapshot);

  /* 标定永远零输出；即使快照失败也要清零旧目标（避免掉线后残留电流）。 */
  (void)Gm6020_SetOutputEnabled(motor, false);
  (void)Gm6020_SetCurrent(motor, 0);
  const bool can_submitted = Gm6020_Send(motor);

  if (!test->prompt_printed ||
      now_ms - test->last_prompt_ms >= prompt_period_ms) {
    const bool prompt_sent = usart_try_printf(
         "[%s标定] 输出命令=零电流\r\n请移动到中位并记录连续角度\r\n"
        "请移动到%s附近并记录，保留机械安全余量\r\n"
        "请移动到%s附近并记录，保留机械安全余量\r\n"
        "记录后填写本轴配置，确认范围后启用标定\r\n",
        test->axis_name, test->first_limit_name, test->second_limit_name);
    if (prompt_sent) {
      test->prompt_printed = true;
      test->last_prompt_ms = now_ms;
    }
  }
  if (!snapshot_valid ||
      (test->log_started && now_ms - test->last_log_ms < log_period_ms)) {
    return;
  }
  const uint32_t age_ms = snapshot.feedback_received &&
                              now_ms - snapshot.feedback.last_feedback_tick <= INT32_MAX
                                  ? now_ms - snapshot.feedback.last_feedback_tick : 0U;
  const bool log_sent = usart_try_printf("[%s标定] 在线=%u 单圈角度=%u 连续角度=%ld 转速=%d "
               "反馈电流=%d 温度=%u 反馈年龄(ms)=%lu 输出命令=0 CAN提交=%u 配置有效=%u\r\n",
               test->axis_name, snapshot.online ? 1U : 0U,
               snapshot.feedback.angle_raw,
               (long)snapshot.feedback.angle_total_raw,
               snapshot.feedback.speed_rpm, snapshot.feedback.current_raw,
               snapshot.feedback.temperature_c, (unsigned long)age_ms,
               can_submitted ? 1U : 0U, (calibration->valid &&
                calibration->min_angle_raw < calibration->center_angle_raw &&
                calibration->center_angle_raw < calibration->max_angle_raw) ? 1U : 0U);
  if (log_sent) {
    test->last_log_ms = now_ms;
    test->log_started = true;
  }
}
