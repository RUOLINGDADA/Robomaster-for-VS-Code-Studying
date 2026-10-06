/**
 * @file low_pass_filter.c
 * @brief 一阶低通滤波。
 *
 * 公式为 y += alpha * (x - y)。输入和输出使用相同单位。
 * 调用者独占实例；任务或串行普通调用均可。不访问外设或判断故障。
 * alpha 越小，噪声越少，响应越慢。滤波不能替代反馈新鲜度检查。
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

/**
 * @brief 初始化输出与滤波系数。
 * @param filter 调用者独占对象；空指针不操作。
 * @param initial_value 初始输出，单位由调用者定义。
 * @param alpha 有限系数；函数钳位到 [0,1]。
 * @retval None；建立滤波历史。
 * @note 串行调用。Init/Reset/Update 不可并发修改同一实例。
 */
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

/**
 * @brief 保留 alpha 并重建输出历史。
 * @param filter 调用者独占对象；空指针不操作。
 * @param value 新输出，单位与采样相同。
 * @retval None；下周期从 value 继续滤波。
 * @note 串行调用；不修改任何硬件输出。
 */
void LowPassFilter_Reset(LowPassFilter_HandleTypeDef *filter, float value) {
  if (filter == NULL) {
    return;
  }
  filter->value = value;
  filter->initialized = true;
}

/**
 * @brief 用当前采样推进一次低通。
 * @param filter 调用者独占对象；未初始化时以本次输入建立历史。
 * @param input 有限采样值；单位须与历史相同。
 * @retval 滤波值；空指针返回 0。
 * @note 串行调用。更小 alpha 会增加响应滞后，不能掩盖反馈过期。
 */
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
