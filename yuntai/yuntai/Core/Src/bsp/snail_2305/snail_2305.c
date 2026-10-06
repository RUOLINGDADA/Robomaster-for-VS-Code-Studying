/**
 * @file snail_2305.c
 * @brief C615/Snail 2305 的双通道 PWM 与脉宽 Ramp。
 *
 * C615 V1.0 手册允许 400~2200 us，最高 500 Hz；本工程使用 50 Hz。
 * PE9/TIM1_CH1 与 PE11/TIM1_CH2 使用 1 MHz 计数、20000 us 周期。
 * 每通道由所属任务独占。先写停止 CCR，再启动 PWM；ISR 不调用本接口。
 * 驱动不修改 CubeMX 时基，不报告电调在线或实际转速。方向由电调或相线设置。
 */
#include "bsp/snail_2305/snail_2305.h"
#include <math.h>
#include <stddef.h>

/* 校验通道、手册脉宽和有限 Ramp 速率，再检查 PSC/ARR；所属任务在初始化前调用。
 * STM32F407 TIM1 时钟为 PCLK2，APB2 分频时为 2×PCLK2。PSC 后须为 1 MHz，ARR 须为 19999。
 * 不满足即拒绝初始化；否则 1000 count 未必是 1000 us，会误设电调油门。 */
static bool Snail2305_ConfigValid(const Snail2305_ConfigTypeDef *config) {
  if (config == NULL || config->htim == NULL || config->htim->Instance != TIM1 ||
      (config->channel != TIM_CHANNEL_1 && config->channel != TIM_CHANNEL_2) ||
      (config->direction_sign != 1 && config->direction_sign != -1) ||
      config->stop_pulse_us < SNAIL_2305_MIN_PROTOCOL_PULSE_US ||
      config->max_pulse_us > SNAIL_2305_MAX_PROTOCOL_PULSE_US ||
      config->stop_pulse_us >= config->max_pulse_us ||
      !isfinite(config->ramp_us_per_s) || config->ramp_us_per_s <= 0.0f) {
    return false;
  }
  const uint32_t pclk_hz = HAL_RCC_GetPCLK2Freq();
  const uint32_t timer_hz = (RCC->CFGR & RCC_CFGR_PPRE2) == 0U ? pclk_hz : 2U * pclk_hz;
  return timer_hz == 1000000U * (config->htim->Instance->PSC + 1U) &&
      config->htim->Instance->ARR == 19999U;
}

/**
 * @brief 校验 TIM1 并以停止脉宽启动一个通道。
 * @param motor 零初始化的持久句柄。每通道只能有一个句柄。
 * @param config 1 MHz、50 Hz 的 TIM1 通道配置。
 * @retval true PWM 已启动。false 配置错误或 HAL 启动失败。
 * @note 仅所属任务调用。失败保留停止 CCR，不修改定时器时基。
 */
bool Snail2305_Init(Snail2305_HandleTypeDef *motor, const Snail2305_ConfigTypeDef *config) {
  if (motor == NULL || !Snail2305_ConfigValid(config)) {
    return false;
  }
  if (motor->initialized) {
    return motor->config.htim == config->htim && motor->config.channel == config->channel;
  }
  motor->config = *config;
  motor->pulse_us = config->stop_pulse_us;
  motor->target_pulse_us = config->stop_pulse_us;
  Ramp_Init(&motor->ramp, (float)config->stop_pulse_us);
  /* 启动前写停止值。若先启动，旧 CCR 可能输出高油门。先停再开。 */
  __HAL_TIM_SET_COMPARE(config->htim, config->channel, config->stop_pulse_us);
  if (HAL_TIM_PWM_Start(config->htim, config->channel) != HAL_OK) {
    return false;
  }
  motor->initialized = true;
  return true;
}

/**
 * @brief 查询 PWM 是否启动成功。
 * @param motor 所属任务的句柄。
 * @retval true 已初始化。false 空指针或未启动。
 * @note 仅所属任务调用。不提供电调在线或实际转速判断。
 */
bool Snail2305_IsInitialized(const Snail2305_HandleTypeDef *motor) {
  return motor != NULL && motor->initialized;
}

/**
 * @brief 立即写脉宽并重置斜坡历史。
 * @param motor 已启动句柄。
 * @param pulse_us 校准范围内的脉宽，us。
 * @retval true 已写 CCR。false 参数非法，旧值不变。
 * @note 仅所属任务调用。CCR 预装载在下个 PWM 周期生效，最迟 20 ms。
 */
bool Snail2305_SetPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us) {
  if (!Snail2305_SetTargetPulse(motor, pulse_us)) {
    return false;
  }
  motor->pulse_us = pulse_us;
  Ramp_Reset(&motor->ramp, (float)pulse_us);
  __HAL_TIM_SET_COMPARE(motor->config.htim, motor->config.channel, pulse_us);
  return true;
}

/**
 * @brief 更新停止脉宽并立即输出该值。
 * @param motor 已启动句柄。
 * @param pulse_us 新的停止脉宽，us；必须位于 400~2200 us 且小于最大脉宽。
 * @retval true 已更新配置并写入 CCR。false 参数非法，旧配置不变。
 * @note 仅所属任务调用。修改后会清除 Ramp 历史，适合校准后的安全停机值。
 */
bool Snail2305_SetStopPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us) {
  if (!Snail2305_IsInitialized(motor) || pulse_us < SNAIL_2305_MIN_PROTOCOL_PULSE_US ||
      pulse_us >= motor->config.max_pulse_us) {
    return false;
  }
  motor->config.stop_pulse_us = pulse_us;
  return Snail2305_SetPulse(motor, pulse_us);
}

/**
 * @brief 设置斜坡目标，不立即跳变输出。
 * @param motor 已启动句柄。
 * @param pulse_us 校准范围内的目标，us。
 * @retval true 已保存目标。false 参数非法，旧目标不变。
 * @note 仅所属任务调用。需要周期调用 Process。
 */
bool Snail2305_SetTargetPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us) {
  if (!Snail2305_IsInitialized(motor) || pulse_us < motor->config.stop_pulse_us ||
      pulse_us > motor->config.max_pulse_us) {
    return false;
  }
  motor->target_pulse_us = pulse_us;
  return true;
}

/**
 * @brief 将千分比换为校准脉宽目标。
 * @param motor 已启动句柄。
 * @param output_permille 0~1000‰。0 为停止，1000 为最大脉宽。
 * @retval true 已保存目标。false 参数非法。
 * @note 仅所属任务调用。千分比不是转速反馈。
 */
bool Snail2305_SetOutput(Snail2305_HandleTypeDef *motor, uint16_t output_permille) {
  if (!Snail2305_IsInitialized(motor) || output_permille > 1000U) {
    return false;
  }
  const uint32_t pulse = motor->config.stop_pulse_us +
      (uint32_t)(motor->config.max_pulse_us - motor->config.stop_pulse_us) * output_permille / 1000U;
  return Snail2305_SetTargetPulse(motor, (uint16_t)pulse);
}

/**
 * @brief 推进一次脉宽斜坡并写入 CCR。
 * @param motor 已启动句柄。
 * @param dt_ms 本周期时长，ms。必须大于 0。
 * @retval true 已写 CCR。false 句柄或周期非法。
 * @note 仅所属任务调用。不延时，不使用定时器中断。
 */
bool Snail2305_Process(Snail2305_HandleTypeDef *motor, uint32_t dt_ms) {
  if (!Snail2305_IsInitialized(motor) || dt_ms == 0U) {
    return false;
  }
  /* 将 ms 换成 s 后按 us/s 推进 Ramp。CCR 预装载到下一次 PWM 更新才生效，最迟 20 ms。
   * pulse_us 只记录最近写值；不能据此断言电调转速已到位。 */
  const float pulse = Ramp_Update(&motor->ramp, (float)motor->target_pulse_us,
                                 motor->config.ramp_us_per_s, (float)dt_ms * 0.001f);
  motor->pulse_us = (uint16_t)lroundf(pulse);
  __HAL_TIM_SET_COMPARE(motor->config.htim, motor->config.channel, motor->pulse_us);
  return true;
}

/**
 * @brief 立即保存停止脉宽，绕过斜坡。
 * @param motor 已启动句柄。
 * @retval true 已写停止 CCR。false 句柄未启动。
 * @note 仅所属任务调用。用于通信故障。PWM 信号保持运行。
 */
bool Snail2305_Stop(Snail2305_HandleTypeDef *motor) {
  /* 通信故障绕过 Ramp 并重置历史。正常松键由运行时设停止目标，保留减速斜坡。 */
  return Snail2305_IsInitialized(motor) && Snail2305_SetPulse(motor, motor->config.stop_pulse_us);
}
