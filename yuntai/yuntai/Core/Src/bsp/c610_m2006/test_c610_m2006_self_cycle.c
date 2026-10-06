/**
 * @file test_c610_m2006_self_cycle.c
 * @brief C610/M2006 非阻塞往返硬件测试实现。
 *
 * 阶段状态只由供弹任务访问。参数由调用者传入。CAN ISR 只更新反馈。
 */
#include "bsp/c610_m2006/test_c610_m2006_self_cycle.h"

#include "app/log/log.h"

#include <stddef.h>
#include <limits.h>

typedef enum {
  C610_TEST_PHASE_WAIT_FEEDBACK = 0, /* 等待新鲜反馈，电流为零。 */
  C610_TEST_PHASE_UP, /* 正向测试阶段。 */
  C610_TEST_PHASE_STOP_AFTER_UP, /* 正向结束后的零电流阶段。 */
  C610_TEST_PHASE_DOWN, /* 反向测试阶段。 */
  C610_TEST_PHASE_STOP_AFTER_DOWN /* 反向结束后的零电流阶段。 */
} C610_TestPhaseTypeDef;

typedef struct {
  C610_TestPhaseTypeDef phase; /* 当前输出阶段；供弹任务独占。 */
  uint32_t phase_start_ms; /* 本阶段起点，HAL ms。 */
  uint32_t last_log_ms; /* 最近成功日志的时刻，HAL ms。 */
  bool initialized; /* 首次调用后为 true；停用时清除。 */
  bool log_started; /* 日志被接受后为 true。 */
} C610_TestStateTypeDef;

static C610_TestStateTypeDef g_c610_test_state; /* 唯一供弹任务独占；ISR 不访问。 */

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
/* 将测试阶段转换为日志文本，避免调试人员解释枚举值。 */
static const char *C610_TestPhaseName(C610_TestPhaseTypeDef phase) {
  switch (phase) {
  case C610_TEST_PHASE_UP: return "上弹";
  case C610_TEST_PHASE_STOP_AFTER_UP: return "上弹停止";
  case C610_TEST_PHASE_DOWN: return "下弹";
  case C610_TEST_PHASE_STOP_AFTER_DOWN: return "下弹停止";
  case C610_TEST_PHASE_WAIT_FEEDBACK:
  default: return "等待反馈";
  }
}
#endif

/* 返回当前阶段时长。等待反馈阶段没有计时。 */
static uint32_t C610_TestPhaseDurationMs(C610_TestPhaseTypeDef phase,
                                         const C610_M2006_TestConfigTypeDef *config) {
  switch (phase) {
  case C610_TEST_PHASE_UP: return config->up_time_ms;
  case C610_TEST_PHASE_DOWN: return config->down_time_ms;
  case C610_TEST_PHASE_STOP_AFTER_UP:
  case C610_TEST_PHASE_STOP_AFTER_DOWN: return config->stop_time_ms;
  case C610_TEST_PHASE_WAIT_FEEDBACK:
  default: return 0U;
  }
}

/* 切换阶段并记录起点。重复调用不重置阶段时间。 */
static void C610_TestSetPhase(C610_TestPhaseTypeDef phase, uint32_t now_ms) {
  if (g_c610_test_state.phase == phase) {
    return;
  }
  g_c610_test_state.phase = phase;
  g_c610_test_state.phase_start_ms = now_ms;
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  (void)LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST, "[供弹测试] 阶段=%s\r\n", C610_TestPhaseName(phase));
#endif
}

/* 检查测试时间和电流参数，失败时调用者保持零输出。 */
static bool C610_TestConfigValid(const C610_M2006_TestConfigTypeDef *config) {
  return config != NULL && config->up_time_ms > 0U && config->stop_time_ms > 0U &&
      config->up_time_ms <= INT32_MAX && config->stop_time_ms <= INT32_MAX &&
      config->down_time_ms > 0U && config->down_time_ms <= INT32_MAX &&
      config->log_period_ms > 0U && config->log_period_ms <= INT32_MAX &&
      config->up_current_raw >= C610_M2006_CURRENT_RAW_MIN &&
      config->up_current_raw <= C610_M2006_CURRENT_RAW_MAX &&
      config->down_current_raw >= C610_M2006_CURRENT_RAW_MIN &&
      config->down_current_raw <= C610_M2006_CURRENT_RAW_MAX;
}

/**
 * @brief 推进一次 C610/M2006 自循环。
 * @param motor 已初始化的 C610/M2006 句柄。
 * @param config 供弹任务提供的测试参数。
 * @param now_ms HAL 毫秒时间。
 * @retval None。参数无效或反馈过期时输出零电流。
 * @note 仅任务上下文调用。每次只推进一个非阻塞阶段。
 */
void C610_M2006_TestSelfCycle_Run(C610_M2006_HandleTypeDef *motor,
                                  const C610_M2006_TestConfigTypeDef *config,
                                  uint32_t now_ms) {
  if (motor == NULL) {
    return;
  }
  /* 先验证参数与许可。失败时清除阶段，防止再次启用后续接旧动作。 */
  if (!C610_TestConfigValid(config) || !config->enabled) {
    g_c610_test_state = (C610_TestStateTypeDef){0};
    (void)C610_M2006_SetOutputEnabled(motor, false);
    (void)C610_M2006_SetCurrent(motor, 0);
    (void)C610_M2006_SendAll(motor->config.hcan);
    return;
  }
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
             C610_TestPhaseDurationMs(g_c610_test_state.phase, config)) {
    switch (g_c610_test_state.phase) {
    case C610_TEST_PHASE_UP: C610_TestSetPhase(C610_TEST_PHASE_STOP_AFTER_UP, now_ms); break;
    case C610_TEST_PHASE_STOP_AFTER_UP: C610_TestSetPhase(C610_TEST_PHASE_DOWN, now_ms); break;
    case C610_TEST_PHASE_DOWN: C610_TestSetPhase(C610_TEST_PHASE_STOP_AFTER_DOWN, now_ms); break;
    case C610_TEST_PHASE_STOP_AFTER_DOWN: C610_TestSetPhase(C610_TEST_PHASE_UP, now_ms); break;
    case C610_TEST_PHASE_WAIT_FEEDBACK:
    default: C610_TestSetPhase(C610_TEST_PHASE_WAIT_FEEDBACK, now_ms); break;
    }
  }

  int16_t target_current_raw = 0;
  if (g_c610_test_state.phase == C610_TEST_PHASE_UP) {
    target_current_raw = config->up_current_raw;
  } else if (g_c610_test_state.phase == C610_TEST_PHASE_DOWN) {
    target_current_raw = config->down_current_raw;
  }
  (void)C610_M2006_SetOutputEnabled(motor, target_current_raw != 0);
  (void)C610_M2006_SetCurrent(motor, target_current_raw);
  (void)C610_M2006_SendAll(motor->config.hcan);

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TEST_ENABLE && LOG_FEED_MOTOR_ENABLE
  if (!g_c610_test_state.log_started ||
      now_ms - g_c610_test_state.last_log_ms >= config->log_period_ms) {
    C610_M2006_FeedbackTypeDef feedback = {0};
    if (C610_M2006_GetFeedback(motor, &feedback)) {
      if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
          "[供弹测试] 阶段=%s 角度=%u 转速=%d 目标电流=%d 反馈电流=%d 输出=%u 保留字节=%u 错误码=%u\r\n",
          C610_TestPhaseName(g_c610_test_state.phase), feedback.angle_raw,
          feedback.speed_rpm, target_current_raw, feedback.current_raw,
          motor->output_enabled ? 1U : 0U, feedback.reserved_raw,
          feedback.error_code)) {
        g_c610_test_state.log_started = true;
        g_c610_test_state.last_log_ms = now_ms;
      }
    }
  }
#endif
}
