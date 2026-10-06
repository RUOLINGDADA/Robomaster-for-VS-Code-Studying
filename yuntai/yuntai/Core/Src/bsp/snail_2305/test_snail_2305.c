/**
 * @file test_snail_2305.c
 * @brief C615 独立双通道 PWM 测试实现。
 *
 * 测试路径绕过 M2006 反馈安全门，只验证 PE9/PE11、TIM1 CCR 和 C615 行程。
 * 运行结束后仍以非阻塞斜坡回到停止脉宽。停止、故障或初始化失败不发送活动值。
 */
#include "bsp/snail_2305/test_snail_2305.h"
#include "app/log/log.h"
#include <stddef.h>

static float Snail2305_TestRampRate(void) {
  return (float)(SNAIL_2305_TEST_ACTIVE_PULSE_US -
                 SNAIL_2305_TEST_STOP_PULSE_US) * 1000.0f /
      (float)SNAIL_2305_TEST_RAMP_TIME_MS;
}

/**
 * @brief 初始化一个独立测试状态和两个 TIM1 PWM 通道。
 * @param test 测试状态，必须保持固定地址。
 * @param htim 已初始化的 TIM1。
 * @retval true 两路 PWM 启动成功；false 参数或 HAL 失败。
 * @note 首次输出为停止脉宽；不初始化 CAN，不需要 M2006 反馈。
 */
bool Snail2305_Test_Init(Snail2305_TestStateTypeDef *test,
                         TIM_HandleTypeDef *htim) {
  if (test == NULL || htim == NULL) {
    return false;
  }
  *test = (Snail2305_TestStateTypeDef){0};
  const Snail2305_ConfigTypeDef left_config = {
      .htim = htim,
      .channel = TIM_CHANNEL_1,
      .direction_sign = SNAIL_2305_TEST_CH1_DIRECTION_SIGN,
      .stop_pulse_us = SNAIL_2305_TEST_STOP_PULSE_US,
      .max_pulse_us = SNAIL_2305_TEST_MAX_PULSE_US,
      .ramp_us_per_s = Snail2305_TestRampRate(),
  };
  Snail2305_ConfigTypeDef right_config = left_config;
  right_config.channel = TIM_CHANNEL_2;
  right_config.direction_sign = SNAIL_2305_TEST_CH2_DIRECTION_SIGN;
  if (!Snail2305_Init(&test->left, &left_config) ||
      !Snail2305_Init(&test->right, &right_config)) {
    (void)Snail2305_Stop(&test->left);
    (void)Snail2305_Stop(&test->right);
    return false;
  }
  test->initialized = true;
  return true;
}

/**
 * @brief 输出测试阶段日志。
 * @param test 测试状态，日志只读取最近 CCR 值。
 * @param now_ms 当前 HAL ms。
 * @param startup 是否处于启动停止脉宽阶段。
 * @param active 是否仍在活动保持阶段。
 * @retval None。日志关闭或 DMA 忙不影响 PWM。
 */
static void Snail2305_Test_Log(const Snail2305_TestStateTypeDef *test,
                               uint32_t now_ms, bool startup, bool active) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  if (!test->log_started || now_ms - test->last_log_ms >= SNAIL_2305_TEST_LOG_PERIOD_MS) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
        "[C615测试] 阶段=%s 左PWM(us)=%u 右PWM(us)=%u\r\n",
        startup ? "启动停止" : (active ? "活动" : "停止"),
        (unsigned)test->left.pulse_us, (unsigned)test->right.pulse_us)) {
      ((Snail2305_TestStateTypeDef *)test)->last_log_ms = now_ms;
      ((Snail2305_TestStateTypeDef *)test)->log_started = true;
    }
  }
#else
  (void)test;
  (void)now_ms;
  (void)startup;
  (void)active;
#endif
}

void Snail2305_Test_RunCycle(Snail2305_TestStateTypeDef *test,
                             uint32_t now_ms, uint32_t dt_ms) {
  if (test == NULL || !test->initialized || dt_ms == 0U) {
    return;
  }
  if (!test->started) {
    test->start_ms = now_ms;
    test->started = true;
  }
  const uint32_t elapsed_ms = now_ms - test->start_ms;
  const bool startup = elapsed_ms < SNAIL_2305_TEST_STARTUP_STOP_TIME_MS;
  const uint32_t active_elapsed_ms = elapsed_ms - SNAIL_2305_TEST_STARTUP_STOP_TIME_MS;
  const bool active = !startup && !test->finished &&
      (SNAIL_2305_TEST_RUN_TIME_MS == 0U ||
       active_elapsed_ms < SNAIL_2305_TEST_RUN_TIME_MS);
  if (!startup && !active) {
    test->finished = true;
  }
  const uint16_t target = active ? SNAIL_2305_TEST_ACTIVE_PULSE_US :
                                   SNAIL_2305_TEST_STOP_PULSE_US;
  (void)Snail2305_SetTargetPulse(&test->left, target);
  (void)Snail2305_SetTargetPulse(&test->right, target);
  (void)Snail2305_Process(&test->left, dt_ms);
  (void)Snail2305_Process(&test->right, dt_ms);
  Snail2305_Test_Log(test, now_ms, startup, active);
}
