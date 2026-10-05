/**
  ******************************************************************************
  * @file    low_pass_filter.c
  * @brief   一阶低通滤波器实现。
  *
  * 公式 y[n] = y[n-1] + alpha × (x[n] - y[n-1])。
  * 通俗理解：把新旧值的差距乘以 alpha，只修正一部分，因此突然出现的毛刺不会直接传到控制器。
  ******************************************************************************
  */

#include "algorithm/filter/low_pass_filter.h"

#include <stddef.h>

/* 将 alpha 限制在 0~1；超出范围会让公式越过新旧值，反而把反馈毛刺放大。 */
static float LowPassFilter_ClampAlpha(float alpha) {
  if (alpha < 0.0f) {
    return 0.0f;
  }
  if (alpha > 1.0f) {
    return 1.0f;
  }
  return alpha;
}

void LowPassFilter_Init(LowPassFilter_HandleTypeDef *filter,
                        float initial_value,
                        float alpha) {
  if (filter == NULL) {
    return;
  }
  filter->value = initial_value;
  filter->alpha = LowPassFilter_ClampAlpha(alpha);
  filter->initialized = true;
}

void LowPassFilter_Reset(LowPassFilter_HandleTypeDef *filter, float value) {
  if (filter == NULL) {
    return;
  }
  filter->value = value;
  filter->initialized = true;
}

float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input) {
  if (filter == NULL) {
    return 0.0f;
  }
  if (!filter->initialized) {
    LowPassFilter_Init(filter, input, filter->alpha);
  }
  
  /*
   * y[n]=y[n-1]+alpha*(x[n]-y[n-1])：只走差值的一部分；若直接赋值，单帧
   * CAN 毛刺会原样进入控制环，alpha 越小越稳但滞后越明显（不能两者同时为零）。
   */
  filter->value += filter->alpha * (input - filter->value);
  return filter->value;
}
