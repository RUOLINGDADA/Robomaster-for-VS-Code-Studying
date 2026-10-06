/**
 * @file test_gm6020_calibration.c
 * @brief GM6020 的手动角度标定硬件测试。
 *
 * 先初始化 CAN 和电机，再由所属轴任务周期调用；每轴独占测试状态。
 * 每周期取一份反馈，清除输出许可并提交零电流。角度使用连续 count。
 * 任务限频输出 UART 日志；DMA 忙时保留提示重试。本模块不自动确认或保存标定。
 */

#include "bsp/gm6020/test_gm6020_calibration.h"

#include "app/log/log.h"

#include <stddef.h>

#define GM6020_CALIBRATION_LOG_PERIOD_MS 200U /* 连续角度日志最短间隔，单位 HAL ms。避免串口刷屏。 */
#define GM6020_CALIBRATION_PROMPT_PERIOD_MS 5000U /**
 * @brief  执行一次非阻塞的手动角度标定。每次只读一份反馈并尝试发零电流。
 * @param  motor 已初始化并注册接收的 GM6020 句柄。函数不会修改标定值。只访问驱动反馈。
 * @param  calibration 该轴标定值，仅用于日志状态提示。不会写回配置。
 * @param  test 该轴独立的测试状态，不能在 Yaw/Pitch 间共享。各轴分别记录限频时间。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。必须与反馈时间戳同源。
 * @note   调用者应在任务初始化电机后每周期调用。test 可先清零，不需要额外 Init。
 * 只能在任务上下文调用。每次调用发送零电流，禁止在 CAN ISR 调用。ISR 只收帧。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
/* 操作提示重发间隔，单位 HAL ms。提醒操作者记录边界。 */

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
  (void)Gm6020_Process(motor, now_ms);
  /* 标定永远零输出。即使快照失败也要清零旧目标。避免掉线后残留电流。 */
  (void)Gm6020_SetOutputEnabled(motor, false);
  (void)Gm6020_SetCurrent(motor, 0);
  const bool can_submitted = Gm6020_Send(motor);
  (void)can_submitted; /* 日志关闭仍发送零电流，不能删除这次 CAN 操作。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && (LOG_YAW_ENABLE || LOG_PITCH_ENABLE)
  if (!LOG_CATEGORY_ENABLED(test->log_category)) {
    return;
  }
  const uint32_t log_period_ms = test->log_period_ms != 0U
      ? test->log_period_ms : GM6020_CALIBRATION_LOG_PERIOD_MS;
  const uint32_t prompt_period_ms = test->prompt_period_ms != 0U
      ? test->prompt_period_ms : GM6020_CALIBRATION_PROMPT_PERIOD_MS;
  Gm6020_SnapshotTypeDef snapshot = {0};
  const bool snapshot_valid = Gm6020_GetSnapshot(motor, &snapshot);



  if (!test->prompt_printed ||
      now_ms - test->last_prompt_ms >= prompt_period_ms) {
    const bool prompt_sent = LOG_TRY_PRINTF(test->log_category,
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
  const bool log_sent = LOG_TRY_PRINTF(test->log_category, "[%s标定] 在线=%u 单圈角度=%u 连续角度=%ld 转速=%d "
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
#endif
}
