/**
 * @file pid.c
 * @brief 离散 PID 与抗积分饱和。
 *
 * 调用者定义误差和输出单位，并传入真实周期 dt_s。
 * 实例由所属控制任务独占。不访问 CAN、PWM 或 RTOS 对象。
 * 输出饱和时禁止积分继续向外累加，防止恢复后过冲。
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

/**
 * @brief 复制 PID 参数并清除控制历史。
 * @param controller 调用者独占对象；空指针不操作。
 * @param kp 比例增益，输出单位/误差单位。
 * @param ki 积分增益，输出单位/(误差单位·s)。
 * @param kd 微分增益，输出单位·s/误差单位。
 * @param output_min 输出下限；须不大于 output_max。
 * @param output_max 输出上限；与下一级输入单位相同。
 * @param integral_min 积分项下限；须不大于 integral_max。
 * @param integral_max 积分项上限；与输出同单位。
 * @retval None；参数由上层保证有限且范围有效。
 * @note 串行调用。每个控制环独占实例，不能共用积分历史。
 */
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

/**
 * @brief 清除积分与微分历史，保留参数。
 * @param controller 调用者独占对象；空指针不操作。
 * @retval None；下次 Update 避免首帧微分尖峰。
 * @note 串行调用。反馈恢复时重置，防止旧积分推动电机。
 */
void Pid_Reset(Pid_ControllerTypeDef *controller) {
  if (controller == NULL) {
    return;
  }
  controller->integral = 0.0f;
  controller->previous_error = 0.0f;
  controller->initialized = false;
}

/**
 * @brief 计算一次限幅后的离散 PID 输出。
 * @param controller 已配置且独占的控制器。
 * @param error 目标减反馈，单位与增益一致；须为有限值。
 * @param dt_s 实际周期，s；须为有限正值。
 * @retval 限幅输出；空指针或非正周期返回 0。
 * @note 串行调用。饱和时只接受有助于离开饱和的积分，防止解除限幅后反冲。
 */
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
