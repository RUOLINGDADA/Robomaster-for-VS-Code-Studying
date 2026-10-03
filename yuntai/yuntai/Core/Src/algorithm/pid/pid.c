/**
  ******************************************************************************
  * @file    pid.c
  * @brief   离散 PID、积分限幅和抗积分饱和实现。
  ******************************************************************************
  */

#include "algorithm/pid/pid.h"

#include <stddef.h>

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
    /* 第一次调用不使用从零开始的巨大微分尖峰。 
    如果不做这个，第一次调用的时候，previous_error=0，误差突然很大，微分项瞬间爆炸，产生巨大冲击。
    第一次直接把上一次误差赋值为当前 error，微分 = 0 */
    controller->previous_error = error;
    controller->initialized = true;
  }
  //误差变化率
  const float derivative = (error - controller->previous_error) / dt_s;
  //先计算后限幅
  const float next_integral = Pid_Clamp(
      controller->integral + controller->ki * error * dt_s,
      controller->integral_min, controller->integral_max);
  const float unsaturated = controller->kp * error + next_integral +
                            controller->kd * derivative;
  const float output =
      Pid_Clamp(unsaturated, controller->output_min, controller->output_max);

  /*
    积分饱和现象：电机被挡住，误差长期很大，积分一直累加，就算误差反向了，积分值很大，输出不能马上回来，会严重超调。
    抗积分饱和逻辑:
    输出已经满功率的时候，不再继续往同一个方向堆积分
   */
  const bool saturated_high = unsaturated > controller->output_max;
  const bool saturated_low = unsaturated < controller->output_min;
  /*
    1. 如果输出没有顶到上下限 → 正常更新积分
    2. 如果输出顶上限（饱和高）：只有误差反向（error<0）的时候，才允许积分更新（让积分往回降）
    3. 如果输出顶下限（饱和低）：只有误差反向（error>0）的时候，才允许积分更新
  */
  if ((!saturated_high && !saturated_low) ||
      (saturated_high && error < 0.0f) ||
      (saturated_low && error > 0.0f)) {
    controller->integral = next_integral;
  }
  controller->previous_error = error;
  return output;
}
