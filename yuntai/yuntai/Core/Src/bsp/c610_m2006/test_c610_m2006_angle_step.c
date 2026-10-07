/**
 * @file test_c610_m2006_angle_step.c
 * @brief C610/M2006 只读手动角度步长测量实现。
 *
 * 测试路径不建立目标角度、不运行位置 P，也不输出非零电流。首帧新鲜反馈
 * 作为测量零点；操作者手动转动拨弹机构，日志实时报告连续角度和相对零点
 * 的 count。它复用正式 C610 驱动和反馈快照，不复制 CAN 协议或控制算法。
 */
#include "bsp/c610_m2006/test_c610_m2006_angle_step.h"

#include "app/log/log.h"

#include <stddef.h>

#define C610_ANGLE_STEP_COUNT_TEXT_SIZE 24U /* int64_t 十进制文本最大长度，含符号和 NUL。 */

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
/**
 * @brief 把连续角度 count 转成与 C 库长整型无关的十进制文本。
 * @param value 需要显示的有符号连续 count。
 * @param text 输出缓冲区，至少 C610_ANGLE_STEP_COUNT_TEXT_SIZE 字节。
 * @retval None。调用者保证缓冲区足够；日志使用 %s，避免目标端 %lld 裁剪支持差异。
 */
static void C610_AngleStep_FormatCount(int64_t value,
                                      char text[C610_ANGLE_STEP_COUNT_TEXT_SIZE]) {
  char reverse[C610_ANGLE_STEP_COUNT_TEXT_SIZE] = {0};
  uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
  size_t length = 0U;
  do {
    reverse[length++] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
  } while (magnitude != 0U && length < sizeof(reverse) - 1U);
  if (value < 0 && length < sizeof(reverse) - 1U) {
    reverse[length++] = '-';
  }
  for (size_t index = 0U; index < length; index++) {
    text[index] = reverse[length - index - 1U];
  }
  text[length] = '\0';
}
#endif

/**
 * @brief 检查手动角度测量配置。
 * @param config task_feed_motor 组装的测试配置。
 * @retval true 参数可用于只读监测；false 时调用者必须清零输出。
 */
static bool C610_AngleStepConfigValid(
    const C610_M2006_AngleStepConfigTypeDef *config) {
  return config != NULL && config->enabled &&
      (config->feedback_sign == 1 || config->feedback_sign == -1) &&
      config->log_period_ms > 0U && config->log_period_ms <= INT32_MAX;
}

/**
 * @brief 清除手动测量状态并回到等待首帧阶段。
 * @param state 任务独占的持久状态。
 * @retval None。
 */
static void C610_AngleStep_Reset(
    C610_M2006_AngleStepStateTypeDef *state) {
  *state = (C610_M2006_AngleStepStateTypeDef){
      .phase = C610_M2006_ANGLE_STEP_WAIT_FEEDBACK};
}

/**
 * @brief 关闭 C610 输出并提交零电流。
 * @param motor 已初始化的 C610/M2006 句柄。
 * @retval true HAL 接受零电流聚合帧；false 句柄未初始化或提交失败。
 * @note 手动测量模式的安全不变量是任何周期都不能给电机驱动电流。
 */
static bool C610_AngleStep_SendZero(C610_M2006_HandleTypeDef *motor) {
  if (motor == NULL || !motor->initialized) {
    return false;
  }
  (void)C610_M2006_SetOutputEnabled(motor, false);
  (void)C610_M2006_SetCurrent(motor, 0);
  return C610_M2006_SendAll(motor->config.hcan);
}

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
/**
 * @brief 按配置周期输出手动角度测量值。
 * @param state 任务独占状态，包含首帧零点和最近增量。
 * @param config 测试日志周期配置。
 * @param feedback 与控制同周期复制的 C610 反馈快照。
 * @param logical_angle 反馈符号转换后的连续角度，count。
 * @param logical_speed 反馈符号转换后的速度，rpm。
 * @param submitted 本周期 HAL 是否接受零电流帧，不表示电调已执行。
 * @param now_ms 当前 HAL 毫秒。
 * @retval true 日志已提交；false 日志关闭、忙或尚未到周期。
 */
static bool C610_AngleStep_Log(
    C610_M2006_AngleStepStateTypeDef *state,
    const C610_M2006_AngleStepConfigTypeDef *config,
    const C610_M2006_FeedbackTypeDef *feedback,
    int64_t logical_angle,
    int32_t logical_speed,
    bool submitted,
    uint32_t now_ms) {
  if (state->log_started && now_ms - state->last_log_ms < config->log_period_ms) {
    return false;
  }
  uint32_t age_ms = now_ms - feedback->last_feedback_tick;
  if (age_ms > INT32_MAX) {
    age_ms = 0U; /* ISR 可能刚更新了略晚于本周期 now_ms 的反馈。 */
  }
  char logical_angle_text[C610_ANGLE_STEP_COUNT_TEXT_SIZE];
  char reference_text[C610_ANGLE_STEP_COUNT_TEXT_SIZE];
  char relative_text[C610_ANGLE_STEP_COUNT_TEXT_SIZE];
  char sample_delta_text[C610_ANGLE_STEP_COUNT_TEXT_SIZE];
  C610_AngleStep_FormatCount(logical_angle, logical_angle_text);
  C610_AngleStep_FormatCount(state->reference_count, reference_text);
  C610_AngleStep_FormatCount(state->delta_from_reference, relative_text);
  C610_AngleStep_FormatCount(state->delta_since_previous, sample_delta_text);
  if (!LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
      "[C610手动角度] 单圈(count)=%u 当前(count)=%s 基准(count)=%s 相对基准(count)=%s 采样增量(count)=%s 转速(rpm)=%ld 反馈电流(raw)=%d 有效=1 年龄(ms)=%lu CAN提交=%u；指令电流=0，输出许可=0，请手动转动\r\n",
      (unsigned)feedback->angle_raw,
      logical_angle_text, reference_text, relative_text, sample_delta_text,
      (long)logical_speed,
      (int)feedback->current_raw, (unsigned long)age_ms, submitted ? 1U : 0U)) {
    return false;
  }
  state->last_log_ms = now_ms;
  state->log_started = true;
  return true;
}
#endif

/**
 * @brief 执行一个手动角度步长测量周期。
 * @param motor 已注册的 C610/M2006 句柄；反馈和发送复用正式驱动。
 * @param state 任务独占的持久测量状态。
 * @param config task_feed_motor 组装的只读测试参数；调用期间保持有效。
 * @param now_ms HAL_GetTick() 当前毫秒。
 * @param dt_ms 保留的任务周期参数，单位 ms；只读测量不使用控制积分。
 * @retval None。始终关闭输出；参数无效、反馈过期或测试关闭时清零状态。
 * @note 只做一个非阻塞周期；不能在 CAN ISR 调用或等待电调响应。
 */
void C610_M2006_AngleStep_RunCycle(
    C610_M2006_HandleTypeDef *motor,
    C610_M2006_AngleStepStateTypeDef *state,
    const C610_M2006_AngleStepConfigTypeDef *config,
    uint32_t now_ms,
    uint32_t dt_ms) {
  (void)dt_ms;
  if (motor == NULL || state == NULL || !C610_AngleStepConfigValid(config)) {
    if (state != NULL) {
      C610_AngleStep_Reset(state);
    }
    (void)C610_AngleStep_SendZero(motor);
    return;
  }

  /* 先关输出并提交零电流，再复制一份快照供角度测量和日志共用。
   * 反馈失效时清除基准；离线期间可能漏过多圈，不能把恢复后的差值当成完整位移。 */
  (void)C610_M2006_Process(motor, now_ms);
  const bool submitted = C610_AngleStep_SendZero(motor);
  (void)submitted; /* 日志裁剪仍须提交零电流。 */

  C610_M2006_FeedbackTypeDef feedback = {0};
  if (!C610_M2006_IsOnline(motor) ||
      !C610_M2006_GetFeedback(motor, &feedback)) {
    C610_AngleStep_Reset(state);
    return;
  }

  const int64_t logical_angle = feedback.angle_total_raw * config->feedback_sign;
  const int32_t logical_speed = (int32_t)feedback.speed_rpm * config->feedback_sign;
  if (!state->initialized) {
    state->phase = C610_M2006_ANGLE_STEP_MONITOR;
    state->reference_count = logical_angle;
    state->previous_count = logical_angle;
    state->delta_from_reference = 0;
    state->delta_since_previous = 0;
    state->initialized = true;
  } else {
    state->delta_since_previous = logical_angle - state->previous_count;
    state->previous_count = logical_angle;
    state->delta_from_reference = logical_angle - state->reference_count;
  }

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  (void)C610_AngleStep_Log(state, config, &feedback, logical_angle,
                           logical_speed, submitted, now_ms);
#else
  (void)feedback;
  (void)logical_speed;
#endif
}
