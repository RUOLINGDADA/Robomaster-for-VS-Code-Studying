/**
 * @file test_gm6020_angle_loop.c
 * @brief GM6020 的固定目标角度硬件测试。
 *
 * 先初始化 GimbalAxis，再由所属轴任务调用；目标为相对中心的 deg。
 * 检查标定和目标范围，随后复用正式闭环、Ramp 与边界过滤。无效目标发零电流。
 * 控制和任务日志共用一次反馈快照。本模块不复制控制算法，操作者须能立即断电。
 */
#include "bsp/gm6020/test_gm6020_angle_loop.h"
#include "app/log/gimbal_log.h"
#include <math.h>
#include <stddef.h>

#define GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS 200U /**
 * @brief  执行一次固定目标角度环硬件调参。沿用正式控制、Ramp 和软件边界过滤。
 * @param  motor 该轴 GM6020 句柄，用于读取本周期一致快照。必须等于 &axis->motor。
 * @param  calibration 该轴中心与边界配置。函数检查它是否仍与 axis 配置一致。
 * @param  axis 正式通用角度控制实例，不能与其它轴共享。测试直接复用这份历史状态。
 * @param  test 该轴独立测试状态和目标角度参数。Yaw/Pitch 不能共用。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。用于反馈年龄和日志限频。
 * @param  dt_ms 控制周期，单位毫秒。必须大于零，传给角度/速度环。
 * @note   先 GimbalAxis_Init(&axis, &config) 注册电机，任务每周期直接调用：
 * Gm6020_TestAngleLoop_Run(&axis.motor, &axis.config.calibration,
 * &axis, &test, HAL_GetTick(), 2U);
 * test 置零，指定轴名称、目标度数和日志间隔。不需要测试 Init。
 * 只在任务上下文调用，禁止 CAN ISR 调用。motor 必须为 &axis.motor。避免读错轴。
 * PID、Ramp 和边界参数改对应轴配置，确认后正式模式使用同一组参数。测试不复制算法。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
/* 未指定日志参数时使用的间隔，单位 ms。避免串口刷屏。 */

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
   * 在转换为 int32_t 前检查范围与 NaN，避免用户宏数值过大触发未定义转换。越界直接零输出。
   * 角度转换使用 double 完成，保留负数/小数目标，最终四舍五入到一个计数。
   */
  const double requested_raw = (double)calibration->center_angle_raw +
      (double)test->target_angle_deg * GM6020_ENCODER_COUNTS_PER_REV / 360.0;
  if (!same_calibration || !isfinite(requested_raw) ||
      requested_raw < INT32_MIN || requested_raw > INT32_MAX) {
    (void)GimbalAxis_SendZero(axis);
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && (LOG_YAW_ENABLE || LOG_PITCH_ENABLE)
    if (!test->target_warning_logged) {
      test->target_warning_logged = LOG_TRY_PRINTF(test->log_category,
          "[%s] 参数不匹配或角度转换无效，保持零输出\r\n", test->axis_name);
    }
#endif
    return;
  }
  const int32_t target_raw = (int32_t)round(requested_raw);
  /*
   * 标定无效、目标越界、反馈掉线均由正式运行时检查并记录零输出周期。
   * 测试不绕过正式安全检查。
   * 运行时仅调用一次 Gm6020_GetSnapshot。日志直接复用 axis->cycle。
   */
  GimbalAxis_RunFixedTarget(axis, target_raw, now_ms, dt_ms);
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && (LOG_YAW_ENABLE || LOG_PITCH_ENABLE)
  if (!LOG_CATEGORY_ENABLED(test->log_category)) {
    return;
  }
  const uint32_t log_period_ms = test->log_period_ms != 0U
      ? test->log_period_ms : GM6020_ANGLE_TEST_DEFAULT_LOG_PERIOD_MS;
  if (!test->log_started ||
      now_ms - test->last_log_ms >= log_period_ms) {
    if (LOG_TRY_PRINTF(test->log_category, GIMBAL_LOG_FORMAT,
        GIMBAL_LOG_ARGS(axis, test->axis_name, 0))) {
      test->last_log_ms = now_ms;
      test->log_started = true;
    }
  }
#endif
}
