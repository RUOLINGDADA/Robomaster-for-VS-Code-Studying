/**
  ******************************************************************************
  * @file    test_c610_m2006_self_cycle.c
  * @brief   C610/M2006 供弹电机硬件自循环调参实现。
  *
  * 状态机每次任务调用只执行一个控制周期。反馈超时或首帧尚未到达时，
  * 通俗理解：它像一个有人值守的往返测试，先确认在线，再上弹、卸力、下弹、卸力。
  * 无条件发送零电流；阶段换向之间插入停止时间，防止机械惯性和旧方向
  * 电流叠加造成冲击。
  ******************************************************************************
  */

#include "bsp/c610_m2006/test_c610_m2006_self_cycle.h"

#include "usart.h"

#include <stdbool.h>
#include <stddef.h>

#if C610_M2006_HARDWARE_TEST_ENABLE
typedef enum {
  C610_TEST_PHASE_WAIT_FEEDBACK = 0,
  C610_TEST_PHASE_UP,
  C610_TEST_PHASE_STOP_AFTER_UP,
  C610_TEST_PHASE_DOWN,
  C610_TEST_PHASE_STOP_AFTER_DOWN
} C610_TestPhaseTypeDef;

typedef struct {
  C610_TestPhaseTypeDef phase; /* 当前阶段（决定输出正电流、零电流还是负电流）。 */
  uint32_t phase_start_ms; /* 当前阶段起点，单位 HAL 毫秒（阶段计时从这里开始）。 */
  uint32_t last_log_ms; /* 上次诊断日志时间，单位 HAL 毫秒（只用于日志限频）。 */
  bool initialized; /* true 表示静态测试状态已建立（首次调用完成一次性初始化）。 */
  bool log_started; /* true 表示已经输出过第一条周期日志（DMA 忙时保留 false，下一周期重试）。 */
} C610_TestStateTypeDef;

static C610_TestStateTypeDef g_c610_test_state;

/* 将测试状态转成日志文本，避免把枚举数字直接输出给调参人员。 */
static const char *C610_TestPhaseName(C610_TestPhaseTypeDef phase) {
  switch (phase) {
  case C610_TEST_PHASE_UP:
    return "上弹";
  case C610_TEST_PHASE_STOP_AFTER_UP:
    return "上弹停止";
  case C610_TEST_PHASE_DOWN:
    return "下弹";
  case C610_TEST_PHASE_STOP_AFTER_DOWN:
    return "下弹停止";
  case C610_TEST_PHASE_WAIT_FEEDBACK:
  default:
    return "等待反馈";
  }
}

/* 返回当前阶段持续时间；停止阶段单独保留，换向前先卸力再反转。 */
static uint32_t C610_TestPhaseDurationMs(C610_TestPhaseTypeDef phase) {
  switch (phase) {
  case C610_TEST_PHASE_UP:
    return C610_M2006_TEST_UP_TIME_MS;
  case C610_TEST_PHASE_DOWN:
    return C610_M2006_TEST_DOWN_TIME_MS;
  case C610_TEST_PHASE_STOP_AFTER_UP:
  case C610_TEST_PHASE_STOP_AFTER_DOWN:
    return C610_M2006_TEST_STOP_TIME_MS;
  case C610_TEST_PHASE_WAIT_FEEDBACK:
  default:
    return 0U;
  }
}

static void C610_TestSetPhase(C610_TestPhaseTypeDef phase,
                              uint32_t now_ms) {
  /* 只在真正换相时重置起点；每周期重复赋值会让阶段永远到不了超时。 */
  if (g_c610_test_state.phase == phase) {
    return;
  }
  g_c610_test_state.phase = phase;
  g_c610_test_state.phase_start_ms = now_ms;
  usart_printf("[供弹测试] 阶段=%s\r\n", C610_TestPhaseName(phase));
}
#endif

void C610_M2006_TestSelfCycle_Run(C610_M2006_HandleTypeDef *motor,
                                  uint32_t now_ms) {
  if (motor == NULL) {
    return;
  }

#if C610_M2006_HARDWARE_TEST_ENABLE
  if (!g_c610_test_state.initialized) {
    g_c610_test_state = (C610_TestStateTypeDef){
        .phase = C610_TEST_PHASE_WAIT_FEEDBACK,
        .phase_start_ms = now_ms,
        .initialized = true,
    };
  }

  (void)C610_M2006_Process(motor, now_ms);
  if (!C610_M2006_IsOnline(motor)) {
    C610_TestSetPhase(C610_TEST_PHASE_WAIT_FEEDBACK, now_ms);
    (void)C610_M2006_SetOutputEnabled(motor, false);
    (void)C610_M2006_SetCurrent(motor, 0);
    (void)C610_M2006_SendAll(motor->config.hcan);
    return;
  }

  if (g_c610_test_state.phase == C610_TEST_PHASE_WAIT_FEEDBACK) {
    C610_TestSetPhase(C610_TEST_PHASE_UP, now_ms);
  } else if (now_ms - g_c610_test_state.phase_start_ms >=
             C610_TestPhaseDurationMs(g_c610_test_state.phase)) {
    switch (g_c610_test_state.phase) {
    case C610_TEST_PHASE_UP:
      C610_TestSetPhase(C610_TEST_PHASE_STOP_AFTER_UP, now_ms);
      break;
    case C610_TEST_PHASE_STOP_AFTER_UP:
      C610_TestSetPhase(C610_TEST_PHASE_DOWN, now_ms);
      break;
    case C610_TEST_PHASE_DOWN:
      C610_TestSetPhase(C610_TEST_PHASE_STOP_AFTER_DOWN, now_ms);
      break;
    case C610_TEST_PHASE_STOP_AFTER_DOWN:
      C610_TestSetPhase(C610_TEST_PHASE_UP, now_ms);
      break;
    case C610_TEST_PHASE_WAIT_FEEDBACK:
    default:
      C610_TestSetPhase(C610_TEST_PHASE_WAIT_FEEDBACK, now_ms);
      break;
    }
  }

  int16_t target_current_raw = 0;
  if (g_c610_test_state.phase == C610_TEST_PHASE_UP) {
    target_current_raw = C610_M2006_TEST_UP_CURRENT_RAW;
  } else if (g_c610_test_state.phase == C610_TEST_PHASE_DOWN) {
    target_current_raw = C610_M2006_TEST_DOWN_CURRENT_RAW;
  }

  (void)C610_M2006_SetOutputEnabled(motor, target_current_raw != 0);
  (void)C610_M2006_SetCurrent(motor, target_current_raw);
  (void)C610_M2006_SendAll(motor->config.hcan);

  if (!g_c610_test_state.log_started ||
      now_ms - g_c610_test_state.last_log_ms >=
          C610_M2006_TEST_LOG_PERIOD_MS) {
    C610_M2006_FeedbackTypeDef feedback = {0};
    if (C610_M2006_GetFeedback(motor, &feedback)) {
      g_c610_test_state.log_started = true;
      g_c610_test_state.last_log_ms = now_ms;
      usart_printf(
          "[供弹测试] 阶段=%s 角度=%u 转速=%d 目标电流=%d 反馈电流=%d "
          "输出=%u 保留字节=%u 错误码=%u\r\n",
          C610_TestPhaseName(g_c610_test_state.phase), feedback.angle_raw,
          feedback.speed_rpm, target_current_raw, feedback.current_raw,
          motor->output_enabled ? 1U : 0U, feedback.reserved_raw,
          feedback.error_code);
    }
  }
#else
  (void)now_ms;
  (void)C610_M2006_SetOutputEnabled(motor, false);
  (void)C610_M2006_SetCurrent(motor, 0);
  (void)C610_M2006_SendAll(motor->config.hcan);
#endif
}
