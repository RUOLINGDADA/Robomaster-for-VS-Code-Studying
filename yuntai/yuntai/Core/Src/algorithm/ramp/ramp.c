/**
  ******************************************************************************
  * @file    ramp.c
  * @brief   目标变化率限制器实现。
  *
  * 通俗理解：先算出这一周期最多能改变多少，再决定只走一步还是直接到达目标。
  ******************************************************************************
  */

#include "algorithm/ramp/ramp.h"

#include <stddef.h>

void Ramp_Init(Ramp_HandleTypeDef *ramp, float initial_value) {
  if (ramp == NULL) {
    return;
  }
  ramp->value = initial_value;
  ramp->initialized = true;
}

void Ramp_Reset(Ramp_HandleTypeDef *ramp, float value) {
  Ramp_Init(ramp, value);
}

float Ramp_Update(Ramp_HandleTypeDef *ramp,
                  float target_value,
                  float max_rate_per_s,
                  float dt_s) {
  if (ramp == NULL || dt_s <= 0.0f || max_rate_per_s < 0.0f) {
    return 0.0f;
  }
  if (!ramp->initialized) {
    Ramp_Init(ramp, target_value);
  }

  /* 变化率乘以真实 dt 才是本周期允许的步长；直接把“每秒”当“每周期”会让周期变化改变电流斜率。 */
  const float maximum_step = max_rate_per_s * dt_s;
  const float difference = target_value - ramp->value; /* 目标和当前值之差（正数要增加，负数要减少）。 */
  if (difference > maximum_step) {
    ramp->value += maximum_step; /* 需要增加很多，本次只增加上限（避免输出突然跳变）。 */
  } else if (difference < -maximum_step) {
    ramp->value -= maximum_step; /* 需要减少很多，本次只减少上限（避免反向冲击）。 */
  } else {
    ramp->value = target_value; /* 差距已经很小，直接到达目标（避免长期留下微小误差）。 */
  }
  return ramp->value;
}
