/**
 * @file task_chassis_runtime.c
 * @brief 单任务完成 Ctrl 模式、四轮快照、纯控制与 CAN1 提交。
 *
 * 只由底盘任务写控制状态。CAN ISR 更新驱动反馈，每轮每周期只复制一次。
 * 命令和反馈使用 HAL ms；Bus-Off 依赖 ABOM，不停止/重置 CAN 外设或 CAN2。
 */
#include "task/task_chassis/task_chassis_runtime.h"
#include "task/task_chassis/task_chassis_command.h"
#include "task/task_chassis/task_chassis_config.h"
#include "app/log/log.h"
#include <stddef.h>
#include <string.h>

static const int8_t g_wheel_sign[CHASSIS_WHEEL_COUNT] = {
  CHASSIS_ID1_SIGN, CHASSIS_ID2_SIGN, CHASSIS_ID3_SIGN, CHASSIS_ID4_SIGN
}; /* 源工程轮序/方向表；反馈和电流同时换到同一逻辑方向，本机仍需实测复核。 */

_Static_assert((CHASSIS_ID1_SIGN == 1 || CHASSIS_ID1_SIGN == -1) &&
               (CHASSIS_ID2_SIGN == 1 || CHASSIS_ID2_SIGN == -1) &&
               (CHASSIS_ID3_SIGN == 1 || CHASSIS_ID3_SIGN == -1) &&
               (CHASSIS_ID4_SIGN == 1 || CHASSIS_ID4_SIGN == -1), "chassis wheel signs must be +/-1");
_Static_assert(CHASSIS_CONTROL_GAP_MS > 0U && CHASSIS_CONTROL_GAP_MS <= INT32_MAX,
               "chassis gap must fit HAL half-range");
_Static_assert(CHASSIS_LOG_PERIOD_MS > 0U && CHASSIS_LOG_PERIOD_MS <= INT32_MAX,
               "chassis telemetry must fit HAL half-range");

/* 所有提交都记录 HAL 结果；忙时只累加诊断，不能阻塞控制周期等待邮箱。 */
static void ChassisTask_Send(ChassisTask_RuntimeTypeDef *runtime) {
  runtime->submitted = C620_M3508_SendAll(&hcan1);
  if (!runtime->submitted) { ++runtime->send_failures; }
}

/* 模式或全局故障先关许可再提交零；HAL 未接受零帧时，后续周期继续阻挡非零。 */
static void ChassisTask_Stop(ChassisTask_RuntimeTypeDef *runtime) {
  ChassisControl_Reset(&runtime->control);
  memset(&runtime->output, 0, sizeof(runtime->output));
  memset(runtime->wheel_online, 0, sizeof(runtime->wheel_online));
  memset(runtime->output_allowed, 0, sizeof(runtime->output_allowed));
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    (void)C620_M3508_SetOutputEnabled(&runtime->motors[i], false);
  }
  runtime->zero_pending = true;
  ChassisTask_Send(runtime);
  if (runtime->submitted) { runtime->zero_pending = false; }
}

/* 日志只读本周期副本；默认关闭分类，所有有硬件副作用的操作都在宏之外完成。 */
static void ChassisTask_Log(ChassisTask_RuntimeTypeDef *runtime, uint32_t now_ms) {
  (void)runtime;
  (void)now_ms;
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_CHASSIS_ENABLE
  if (now_ms - runtime->last_log_ms < CHASSIS_LOG_PERIOD_MS) { return; }
  uint32_t online_mask = 0, allowed_mask = 0;
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    if (runtime->feedback[i].online) { online_mask |= 1U << i; }
    if (runtime->output_allowed[i]) { allowed_mask |= 1U << i; }
  }
  const bool accepted = LOG_TRY_PRINTF(LOG_CATEGORY_CHASSIS,
      "[底盘] 模式=%u 在线位=%lu 许可位=%lu CAN已提交=%u 零帧待交=%u BOFF=%u 失败=%lu "
      "目标rpm=%ld,%ld,%ld,%ld 反馈原始rpm=%d,%d,%d,%d "
      "计算电流raw=%d,%d,%d,%d 反馈年龄ms=%lu,%lu,%lu,%lu\r\n",
      (unsigned)runtime->mode, (unsigned long)online_mask, (unsigned long)allowed_mask,
      runtime->submitted ? 1U : 0U, runtime->zero_pending ? 1U : 0U, runtime->was_bus_off ? 1U : 0U,
      (unsigned long)runtime->send_failures,
      (long)runtime->output.target_speed_rpm[0], (long)runtime->output.target_speed_rpm[1],
      (long)runtime->output.target_speed_rpm[2], (long)runtime->output.target_speed_rpm[3],
      (int)runtime->feedback[0].speed_rpm, (int)runtime->feedback[1].speed_rpm,
      (int)runtime->feedback[2].speed_rpm, (int)runtime->feedback[3].speed_rpm,
      (int)runtime->output.current_raw[0], (int)runtime->output.current_raw[1],
      (int)runtime->output.current_raw[2], (int)runtime->output.current_raw[3],
      (unsigned long)runtime->feedback[0].age_ms, (unsigned long)runtime->feedback[1].age_ms,
      (unsigned long)runtime->feedback[2].age_ms, (unsigned long)runtime->feedback[3].age_ms);
  if (accepted) { runtime->last_log_ms = now_ms; }
#endif
}

/* 四轮注册属于一次初始化事务，任一失败撤销本运行时已注册的轮，允许下周期重试。 */
bool ChassisTask_RuntimeInit(ChassisTask_RuntimeTypeDef *runtime) {
  if (runtime == NULL) { return false; }
  if (runtime->ready) { return true; }
  const ChassisControl_ConfigTypeDef config = {
    .max_speed_rpm = CHASSIS_MAX_SPEED_RPM,
    .kp = CHASSIS_SPEED_KP, .ki = CHASSIS_SPEED_KI, .kd = CHASSIS_SPEED_KD,
    .integral_limit_raw = CHASSIS_INTEGRAL_LIMIT_RAW,
    .integral_error_scale = CHASSIS_INTEGRAL_ERROR_SCALE,
    .current_limit_raw = CHASSIS_CURRENT_LIMIT_RAW,
    .stick_deadband_raw = CHASSIS_STICK_DEADBAND_RAW,
    .ramp_t1_s = CHASSIS_KEYBOARD_T1_MS * 0.001f,
    .ramp_t3_s = CHASSIS_KEYBOARD_T3_MS * 0.001f,
    .ramp_linear_ratio = CHASSIS_KEYBOARD_LINEAR_RATIO,
    .ramp_log_shape = CHASSIS_KEYBOARD_LOG_SHAPE,
  };
  if (!ChassisControl_Init(&runtime->control, &config)) { return false; }
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    const C620_M3508_ConfigTypeDef motor = {
      .hcan = &hcan1, .motor_id = (uint8_t)(i + 1U), .feedback_timeout_ms = CHASSIS_FEEDBACK_TIMEOUT_MS,
    };
    if (!C620_M3508_Init(&runtime->motors[i], &motor)) {
      for (uint32_t j = 0; j < i; ++j) { C620_M3508_DeInit(&runtime->motors[j]); }
      ChassisControl_Reset(&runtime->control);
      return false;
    }
  }
  runtime->mode = CHASSIS_MODE_STOP;
  runtime->ready = true;
  runtime->clock_valid = false;
  runtime->zero_pending = true;
  return true;
}

/* 先取得输入并消费模式事件，再复制每轮反馈，最后只由一个分支决定本周期输出。
 * 所有早退都提交零并记录本周期日志；Bus-Off 后清 received，不能拿故障前帧重新使能。 */
void ChassisTask_RuntimeRunCycle(ChassisTask_RuntimeTypeDef *runtime, uint32_t now_ms) {
  if (runtime == NULL || !runtime->ready) { return; }
  Chassis_CommandTypeDef command = {0};
  (void)ChassisCommand_GetSnapshot(now_ms, &command);
  bool changed = false;
  if (command.link_generation != runtime->link_generation) {
    runtime->link_generation = command.link_generation;
    runtime->last_presses = 0;
    runtime->mode = CHASSIS_MODE_STOP;
    changed = true;
  }
  if (!command.enabled) {
    runtime->mode = CHASSIS_MODE_STOP;
    runtime->last_presses = command.mode_presses;
  } else {
    const uint32_t presses = command.mode_presses - runtime->last_presses;
    if (presses != 0U) {
      runtime->mode = (Chassis_ModeTypeDef)(((uint32_t)runtime->mode + presses % 3U) % 3U);
      runtime->last_presses = command.mode_presses;
      changed = true;
    }
  }
  const bool bus_off = hcan1.Instance == NULL || (hcan1.Instance->ESR & CAN_ESR_BOFF) != 0U;
  const bool recovering = runtime->was_bus_off && !bus_off;
  runtime->was_bus_off = bus_off;
  if (bus_off || recovering) {
    for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
      C620_M3508_InvalidateFeedback(&runtime->motors[i]);
    }
  }
  ChassisControl_FeedbackTypeDef feedback[CHASSIS_WHEEL_COUNT] = {0};
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    const bool copied = C620_M3508_GetSnapshot(&runtime->motors[i], &runtime->feedback[i]);
    const bool online = copied && runtime->feedback[i].online;
    if (online != runtime->wheel_online[i]) { Pid_Reset(&runtime->control.speed_pid[i]); }
    runtime->wheel_online[i] = online;
    feedback[i].online = online;
    /* 浮点域变换 INT16_MIN，不在 int16_t 内直接取反。 */
    feedback[i].speed_rpm = online ? (float)runtime->feedback[i].speed_rpm * g_wheel_sign[i] : 0;
  }
  const uint32_t elapsed_ms = now_ms - runtime->previous_ms;
  const bool clock_ok = runtime->clock_valid && elapsed_ms > 0U && elapsed_ms <= CHASSIS_CONTROL_GAP_MS;
  runtime->previous_ms = now_ms;
  runtime->clock_valid = true;
  if (!command.enabled || runtime->mode == CHASSIS_MODE_STOP || changed || !clock_ok ||
      bus_off || recovering || runtime->zero_pending) {
    ChassisTask_Stop(runtime);
    ChassisTask_Log(runtime, now_ms);
    return;
  }
  if (!ChassisControl_Update(&runtime->control, runtime->mode, command.vx_raw, command.vy_raw,
                             command.keyboard, feedback, elapsed_ms * 0.001f, &runtime->output)) {
    ChassisTask_Stop(runtime);
    ChassisTask_Log(runtime, now_ms);
    return;
  }
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    runtime->output_allowed[i] = C620_M3508_SetOutputEnabled(&runtime->motors[i], feedback[i].online) && feedback[i].online;
    const int16_t raw = runtime->output_allowed[i] ? (int16_t)(runtime->output.current_raw[i] * g_wheel_sign[i]) : 0;
    (void)C620_M3508_SetCurrent(&runtime->motors[i], raw);
  }
  ChassisTask_Send(runtime);
  ChassisTask_Log(runtime, now_ms);
}
