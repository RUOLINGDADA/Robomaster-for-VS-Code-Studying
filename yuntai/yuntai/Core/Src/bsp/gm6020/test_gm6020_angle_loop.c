/**
  ******************************************************************************
  * @file    test_gm6020_angle_loop.c
  * @brief   双轴固定角度上板调参适配，不创建或复制第二套控制器。
  ******************************************************************************
  */
#include "bsp/gm6020/test_gm6020_angle_loop.h"
#include "usart.h"
#include <math.h>
#include <stddef.h>

#define GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS 200U /* 未指定日志参数时使用的间隔 ms。 */

void Gm6020_TestAngleLoop_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    GimbalAxis_HandleTypeDef *axis,
    Gm6020_TestAngleLoopTypeDef *test,
    uint32_t now_ms, uint32_t dt_ms) {
  if (axis == NULL) {
    return;
  }
  if (motor == NULL || calibration == NULL || test == NULL ||
      motor != &axis->motor) {
    (void)GimbalAxis_SendZero(axis);
    return;
  }
  if (test->axis_name == NULL) {
    test->axis_name = "云台轴角度测试";
  }
  const Gm6020_CalibrationTypeDef *configured = &axis->config.calibration;
  const bool same_calibration = calibration->valid == configured->valid &&
      calibration->center_angle_raw == configured->center_angle_raw &&
      calibration->min_angle_raw == configured->min_angle_raw &&
      calibration->max_angle_raw == configured->max_angle_raw;
  /*
   * 在转换为 int32_t 前检查范围与 NaN，避免用户宏数值过大触发未定义转换。
   * 角度转换使用 double 完成，保留负数/小数目标，最终四舍五入到一个计数。
   */
  const double requested_raw = (double)calibration->center_angle_raw +
      (double)test->target_angle_deg * GM6020_ENCODER_COUNTS_PER_REV / 360.0;
  if (!same_calibration || !isfinite(requested_raw) ||
      requested_raw < INT32_MIN || requested_raw > INT32_MAX) {
    (void)GimbalAxis_SendZero(axis);
    if (!test->target_warning_logged) {
      test->target_warning_logged = usart_try_printf(
          "[%s] 参数不匹配或角度转换无效，保持零输出\r\n", test->axis_name);
    }
    return;
  }
  const int32_t target_raw = (int32_t)round(requested_raw);
  /*
   * 标定无效、目标越界、反馈掉线均由正式运行时检查并记录零输出周期。
   * 运行时仅调用一次 Gm6020_GetSnapshot；日志直接复用 axis->cycle。
   */
  GimbalAxis_RunFixedTarget(axis, target_raw, now_ms, dt_ms);
  const uint32_t log_period_ms = test->log_period_ms != 0U
      ? test->log_period_ms : GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS;
  if (axis->event_name != NULL || !test->log_started ||
      now_ms - test->last_log_ms >= log_period_ms) {
    if (GimbalAxis_TryLog(axis, test->axis_name, 0)) {
      test->last_log_ms = now_ms;
      test->log_started = true;
    }
  }
}
