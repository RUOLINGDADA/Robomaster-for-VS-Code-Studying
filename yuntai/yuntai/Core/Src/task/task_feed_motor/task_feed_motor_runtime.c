/**
  ******************************************************************************
  * @file    task_feed_motor_runtime.c
  * @brief   供弹电机运行时组合实现。
  *
  * 通俗理解：这里把反馈检查、阶段状态和 CAN 发送串成一个完整周期。
  ******************************************************************************
  */

#include "FreeRTOS.h"

#include "task/task_feed_motor/task_feed_motor_runtime.h"

#include "can.h"
#include "usart.h"

#include <stddef.h>

/* 只从本周期复制的反馈和运行时状态打印；日志限频不能阻塞控制或改写 DMA 缓冲区。 */
static void FeedMotorRuntime_Log(FeedMotor_RuntimeTypeDef *runtime,
                                 const C610_M2006_FeedbackTypeDef *feedback,
                                 bool online,
                                 uint32_t now_ms,
                                 uint32_t now_tick) {
  if (runtime == NULL || feedback == NULL) {
    return;
  }

  /* 日志配置是毫秒；限频比较使用任务 Tick，必须先统一单位（不能直接相减）。 */
  const uint32_t log_period_tick =
      (uint32_t)pdMS_TO_TICKS(runtime->command.log_period_ms);
  if (now_tick - runtime->last_log_tick < log_period_tick) {
    return;
  }
  runtime->last_log_tick = now_tick;

  const uint32_t feedback_age_ms =
      runtime->motor.feedback_received
          ? now_ms - feedback->last_feedback_tick
          : 0U;
  usart_printf(
      "[供弹] 在线=%u 反馈有效=%u 阶段=%s 角度=%u 转速=%d "
      "反馈电流=%d 目标电流=%d 输出=%u 反馈年龄=%lu "
      "保留字节=%u 错误码=%u\r\n",
      online ? 1U : 0U, runtime->motor.feedback_received ? 1U : 0U,
      FeedMotorControl_PhaseName(runtime->control.phase), feedback->angle_raw,
      feedback->speed_rpm, feedback->current_raw,
      runtime->motor.target_current_raw,
       runtime->motor.output_enabled ? 1U : 0U, (unsigned long)feedback_age_ms,
       feedback->reserved_raw, feedback->error_code);
}

bool FeedMotor_RuntimeInit(FeedMotor_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  *runtime = (FeedMotor_RuntimeTypeDef){0};
  FeedMotorCommand_GetDefault(&runtime->command);

  const C610_M2006_ConfigTypeDef motor_config = {
      .hcan = &hcan1,
      .motor_id = runtime->command.motor_id,
      .feedback_timeout_ms = C610_M2006_FEEDBACK_TIMEOUT_MS,
  };
  if (!C610_M2006_Init(&runtime->motor, &motor_config)) {
    return false;
  }
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, false);
  FeedMotorControl_Init(&runtime->control, 0U);
  runtime->initialized = true;
  return true;
}

void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime,
                               uint32_t now_ms,
                               uint32_t now_tick) {
  if (runtime == NULL || !runtime->initialized) {
    return;
  }

  (void)C610_M2006_Process(&runtime->motor, now_ms);
  const bool online = C610_M2006_IsOnline(&runtime->motor);
  C610_M2006_FeedbackTypeDef feedback = {0};
  const bool feedback_valid = C610_M2006_GetFeedback(&runtime->motor, &feedback);

  /*
   * 正式供弹命令适配层尚未接入（当前没有上层命令来源）。即使反馈在线，也不能把测试配置当成
   * 正式命令继续驱动电机；正式路径当前明确保持零输出，防止关闭测试宏
   * 后意外重新执行旧的上弹/下弹自循环。未来的遥控器或上层命令应在这里
   * 经过 FeedMotorControl 后再设置目标电流。
   * 通俗理解：关闭测试模式后默认仍然安全停机，不会偷偷把旧的上弹参数当成正式命令。
   */
  FeedMotorControl_SetWait(&runtime->control, now_tick);
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, false);
  (void)C610_M2006_SetCurrent(&runtime->motor, 0);

  /* C610 控制电流不是永久寄存器，必须每周期刷新聚合帧；不发送就不会保持上一条命令。 */
  (void)C610_M2006_SendAll(&hcan1);
  if (feedback_valid) {
    FeedMotorRuntime_Log(runtime, &feedback, online, now_ms, now_tick);
  }
}
