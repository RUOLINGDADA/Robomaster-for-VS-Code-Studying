/**
  ******************************************************************************
  * @file    low_pass_filter.h
  * @brief   一阶离散低通滤波器。
  *
  * 速度和反馈电流来自 CAN 离散采样，直接用单帧值判定堵转会把量化噪声
  * 当成真实运动。该模块只做 y[n] = y[n-1] + alpha*(x-y) 的数值处理，
  * 不负责选择 alpha，也不负责判断故障。
  *
  * 通俗理解：不要因为某一帧反馈突然抖了一下就立刻改变控制输出，
  * 而是让结果向新采样靠近一小步。
  ******************************************************************************
  */

#ifndef ALGORITHM_LOW_PASS_FILTER_H
#define ALGORITHM_LOW_PASS_FILTER_H /* 防止低通滤波接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float value; /* 滤波后的输出值（下一次计算会从这个历史结果继续）。 */
  float alpha; /* 滤波系数 0~1（越小越平滑，越大越相信新采样）。 */
  bool initialized; /* 是否已经建立初始输出（第一次更新可直接使用输入）。 */
} LowPassFilter_HandleTypeDef;

/**
 * @brief  初始化一阶低通滤波器（建立已知历史，避免首帧沿用未初始化数据）。
 * @param  filter 滤波器实例指针（可以同时创建多个互不干扰的滤波器）。
 * @param  initial_value 初始输出值，单位由调用者定义（决定第一次更新从哪里开始）。
 * @param  alpha 滤波系数，函数限制到 0~1（超范围会让结果越过新旧值并放大毛刺）。
 */
void LowPassFilter_Init(LowPassFilter_HandleTypeDef *filter,
                        float initial_value,
                        float alpha);

/**
 * @brief  保留滤波系数并把输出重置到指定值（清除旧的平滑历史）。
 * @param  filter 滤波器状态（调用者独占）。
 * @param  value 新的输出值，单位由调用者定义（通常填当前实测值）。
 */
void LowPassFilter_Reset(LowPassFilter_HandleTypeDef *filter,
                         float value);

/**
 * @brief  输入一份采样并返回当前滤波结果（只移动 alpha 所占的一部分）。
 * @param  filter 已初始化或可延迟初始化的滤波器（保存上一次结果）。
 * @param  input 当前采样，单位由调用者定义（必须和历史值同单位）。
 * @retval 低通后的数值；传入空指针时返回 0（空指针按安全值处理）。
 */
float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input);

#endif /* ALGORITHM_LOW_PASS_FILTER_H（防止头文件被重复包含） */
