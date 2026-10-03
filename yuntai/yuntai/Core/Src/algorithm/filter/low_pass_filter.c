/**
  ******************************************************************************
  * @file    low_pass_filter.c
  * @brief   一阶低通滤波器实现。
  ******************************************************************************
  */

#include "algorithm/filter/low_pass_filter.h"

#include <stddef.h>

//裁剪 alpha 到 0~1
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
  新滤波值 = 老滤波值 + 系数 ×（新采样值 和老滤波值的差值）
  只取一小部分差值更新，不让输出一下子跳变 → 滤掉突变毛刺
  */
  filter->value += filter->alpha * (input - filter->value);
  return filter->value;
}
