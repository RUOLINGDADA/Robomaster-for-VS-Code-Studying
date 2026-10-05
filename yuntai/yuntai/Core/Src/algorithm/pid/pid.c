/**
  ******************************************************************************
  * @file    pid.c
  * @brief   离散 PID、积分限幅和抗积分饱和实现。
  *
  * 通俗理解：比例项负责“现在纠偏”，积分项负责“补长期欠账”，微分项负责
  * “看变化趋势”；输出顶到上下限时暂缓继续累积积分，避免解除限幅后猛冲。
  ******************************************************************************
  */

#include "algorithm/pid/pid.h"

#include <stddef.h>

/* 限制中间项或最终输出；积分和输出分开限幅，避免历史误差绕过电流安全边界。 */
static float Pid_Clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

void Pid_Init(Pid_ControllerTypeDef *controller,
              float kp,
              float ki,
              float kd,
              float output_min,
              float output_max,
              float integral_min,
              float integral_max) {
  if (controller == NULL) {
    return;
  }
  controller->kp = kp;
  controller->ki = ki;
  controller->kd = kd;
  controller->output_min = output_min;
  controller->output_max = output_max;
  controller->integral_min = integral_min;
  controller->integral_max = integral_max;
  Pid_Reset(controller);
}

void Pid_Reset(Pid_ControllerTypeDef *controller) {
  if (controller == NULL) {
    return;
  }
  controller->integral = 0.0f;
  controller->previous_error = 0.0f;
  controller->initialized = false;
}

float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s) {
  if (controller == NULL || dt_s <= 0.0f) {
    return 0.0f;
  }

  if (!controller->initialized) {
    /* 首次没有真实的上一帧误差；若拿默认 0 做差分，启动瞬间会制造假的微分尖峰。 */
    controller->previous_error = error;
    controller->initialized = true;
  }
  /* 微分项看误差变化率；dt_s 必须是真实周期，否则任务抖动会被当成快速运动。 */
  const float derivative = (error - controller->previous_error) / dt_s;
  /* 先计算候选积分，再限幅（避免历史误差超过允许范围）。 */
  const float next_integral = Pid_Clamp(
      controller->integral + controller->ki * error * dt_s,
      controller->integral_min, controller->integral_max);
  const float unsaturated = controller->kp * error + next_integral +
                            controller->kd * derivative;
  const float output =
      Pid_Clamp(unsaturated, controller->output_min, controller->output_max);

  /*
   * 积分饱和：电机被挡住时误差长期存在，积分会越堆越大，解除阻挡后就会过冲。
   * 这里记录输出是否已经顶到上限或下限（控制器已经没有更多“力气”可用）。
   */
  const bool saturated_high = unsaturated > controller->output_max;
  const bool saturated_low = unsaturated < controller->output_min;
  /*
   * 没有饱和时正常保存积分；已经顶到上限时只接受负误差，顶到下限时只接受正误差
   * （通俗理解：只允许积分把输出从“顶住的方向”拉回来，不允许继续往墙上加力）。
   */
  if ((!saturated_high && !saturated_low) ||
      (saturated_high && error < 0.0f) ||
      (saturated_low && error > 0.0f)) {
    controller->integral = next_integral;
  }
  controller->previous_error = error;
  return output;
}
