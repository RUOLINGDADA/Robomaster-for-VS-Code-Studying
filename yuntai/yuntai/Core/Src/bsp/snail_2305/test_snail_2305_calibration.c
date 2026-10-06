/**
 * @file test_snail_2305_calibration.c
 * @brief C615 PWM 行程和转向切换校准状态机。
 *
 * 最大脉宽阶段用于等待手册中的 BB/BBB 窗口；随后立即切换最小脉宽。
 * 状态机不判断声音，也不自动宣布电调校准成功；最小脉宽阶段结束只表示
 * 软件时序已完成。实际电机方向和行程必须通过电调鸣音与台架确认。
 */
#include "bsp/snail_2305/test_snail_2305_calibration.h"
#include "app/log/log.h"
#include <stddef.h>

static const char *Snail2305_CalibrationModeName(void) {
#if FEED_MOTOR_SNAIL_MODE == FEED_MOTOR_SNAIL_MODE_PWM_CALIBRATION
  return "PWM行程校准";
#elif FEED_MOTOR_SNAIL_MODE == FEED_MOTOR_SNAIL_MODE_DIRECTION_CALIBRATION
  return "电机转向切换";
#else
  return "未启用";
#endif
}

static void Snail2305_CalibrationLog(Snail2305_CalibrationStateTypeDef *state,
                                     uint32_t now_ms, const char *text,
                                     bool *logged) {
  (void)now_ms;
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  if (!*logged && LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
      "[C615校准] 模式=%s %s CH1=%uus CH2=%uus\r\n", Snail2305_CalibrationModeName(),
      text, (unsigned)state->ch1.pulse_us, (unsigned)state->ch2.pulse_us)) {
    *logged = true;
  }
#else
  (void)state;
  (void)text;
  (void)logged;
#endif
}

bool Snail2305_Calibration_Init(Snail2305_CalibrationStateTypeDef *state,
                                TIM_HandleTypeDef *htim) {
  if (state == NULL || htim == NULL) {
    return false;
  }
  *state = (Snail2305_CalibrationStateTypeDef){0};
  const Snail2305_ConfigTypeDef ch1_config = {
      .htim = htim,
      .channel = TIM_CHANNEL_1,
      .direction_sign = SNAIL_2305_CALIBRATION_CH1_DIRECTION_SIGN,
      .stop_pulse_us = SNAIL_2305_CALIBRATION_MIN_PULSE_US,
      .max_pulse_us = SNAIL_2305_CALIBRATION_MAX_PULSE_US,
      .ramp_us_per_s = 1.0f,
  };
  Snail2305_ConfigTypeDef ch2_config = ch1_config;
  ch2_config.channel = TIM_CHANNEL_2;
  ch2_config.direction_sign = SNAIL_2305_CALIBRATION_CH2_DIRECTION_SIGN;
  if (!Snail2305_Init(&state->ch1, &ch1_config) ||
      !Snail2305_Init(&state->ch2, &ch2_config) ||
      !Snail2305_SetPulse(&state->ch1, SNAIL_2305_CALIBRATION_MAX_PULSE_US) ||
      !Snail2305_SetPulse(&state->ch2, SNAIL_2305_CALIBRATION_MAX_PULSE_US)) {
    (void)Snail2305_Stop(&state->ch1);
    (void)Snail2305_Stop(&state->ch2);
    return false;
  }
  state->phase = SNAIL_2305_CALIBRATION_PHASE_MAX;
  state->initialized = true;
  Snail2305_CalibrationLog(state, 0U, "已输出最大脉宽，请等待对应 BB/BBB 窗口", &state->prompt_max_logged);
  return true;
}

void Snail2305_Calibration_RunCycle(Snail2305_CalibrationStateTypeDef *state,
                                    uint32_t now_ms, uint32_t dt_ms) {
  if (state == NULL || !state->initialized || dt_ms == 0U) {
    return;
  }
  if (!state->started) {
    state->phase_start_ms = now_ms;
    state->started = true;
  }
  const uint32_t elapsed_ms = now_ms - state->phase_start_ms;
  if (state->phase == SNAIL_2305_CALIBRATION_PHASE_MAX &&
      elapsed_ms >= SNAIL_2305_CALIBRATION_MAX_HOLD_MS) {
    (void)Snail2305_SetPulse(&state->ch1, SNAIL_2305_CALIBRATION_MIN_PULSE_US);
    (void)Snail2305_SetPulse(&state->ch2, SNAIL_2305_CALIBRATION_MIN_PULSE_US);
    state->phase = SNAIL_2305_CALIBRATION_PHASE_MIN;
    state->phase_start_ms = now_ms;
    Snail2305_CalibrationLog(state, now_ms, "已切换最小脉宽，请等待约 1 秒 B 声并保持", &state->prompt_min_logged);
  } else if (state->phase == SNAIL_2305_CALIBRATION_PHASE_MIN &&
             elapsed_ms >= SNAIL_2305_CALIBRATION_MIN_HOLD_MS) {
    state->phase = SNAIL_2305_CALIBRATION_PHASE_DONE;
    Snail2305_CalibrationLog(state, now_ms, "时序完成，继续保持最小脉宽；请断电确认结果", &state->prompt_done_logged);
  }
}
