/**
  ******************************************************************************
  * @file    low_pass_filter.h
  * @brief   一阶离散低通滤波器。
  *
  * 速度和反馈电流来自 CAN 离散采样，直接用单帧值判定堵转会把量化噪声
  * 当成真实运动。该模块只做 y[n] = y[n-1] + alpha*(x-y) 的数值处理，
  * 不负责选择 alpha，也不负责判断故障。
  ******************************************************************************
  */

#ifndef ALGORITHM_LOW_PASS_FILTER_H
#define ALGORITHM_LOW_PASS_FILTER_H /* 防止低通滤波接口被重复包含。 */

#include <stdbool.h>

typedef struct {
  float value;// 滤波后的输出值（上一次结果）
  float alpha;// 滤波系数 0~1
  bool initialized;
} LowPassFilter_HandleTypeDef;

/*
- filter：滤波器实例指针（可以同时创建多个独立滤波器）
- initial_value：滤波初始值
- alpha：滤波系数，函数会限制到 0~1
*/
void LowPassFilter_Init(LowPassFilter_HandleTypeDef *filter,
                        float initial_value,
                        float alpha);
                        
/**
 * @brief  保留滤波系数并把输出重置到指定值。
 * @param  filter 滤波器状态。
 * @param  value 新的输出值，单位由调用者定义。
 */
void LowPassFilter_Reset(LowPassFilter_HandleTypeDef *filter,
                         float value);

/**
 * @brief  输入一份采样并返回当前滤波结果。
 * @param  filter 已初始化或可延迟初始化的滤波器。
 * @param  input 当前采样，单位由调用者定义。
 * @retval 低通后的数值；传入空指针时返回 0。
 */
float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input);

#endif /* ALGORITHM_LOW_PASS_FILTER_H */
