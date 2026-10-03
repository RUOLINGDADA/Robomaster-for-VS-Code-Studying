/**
  ******************************************************************************
  * @file    ramp.c
  * @brief   目标变化率限制器实现。
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

  const float maximum_step = max_rate_per_s * dt_s;//本次最多能变多少
  const float difference = target_value - ramp->value;//目标和当前ramp值之差
  if (difference > maximum_step) {
    ramp->value += maximum_step;// 需要涨很多 → 本次只涨一步上限
  } else if (difference < -maximum_step) {
    ramp->value -= maximum_step;// 需要降很多 → 本次只降一步上限
  } else {
    ramp->value = target_value;//差距很小，直接到位
  }
  return ramp->value;
}
