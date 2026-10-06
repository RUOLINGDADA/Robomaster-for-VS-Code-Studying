/**
 * @file low_pass_filter.h
 * @brief 一阶低通滤波。
 *
 * 公式为 y += alpha * (x - y)。输入和输出使用相同单位。
 * 调用者独占实例；任务或串行普通调用均可。不访问外设或判断故障。
 * alpha 越小，噪声越少，响应越慢。滤波不能替代反馈新鲜度检查。
 */

#ifndef ALGORITHM_LOW_PASS_FILTER_H
#define ALGORITHM_LOW_PASS_FILTER_H /* 防止低通滤波接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float value; /* 输出历史，与 input 同单位；所属任务更新，Reset 重建。 */
  float alpha; /* 无量纲权重 [0,1]；Init 钳位，越小越平滑且滞后越大。 */
  bool initialized; /* 已建立输出历史；false 时首帧直接采用 input。 */
} LowPassFilter_HandleTypeDef;

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
                        float alpha);

/**
 * @brief 保留 alpha 并重建输出历史。
 * @param filter 调用者独占对象；空指针不操作。
 * @param value 新输出，单位与采样相同。
 * @retval None；下周期从 value 继续滤波。
 * @note 串行调用；不修改任何硬件输出。
 */
void LowPassFilter_Reset(LowPassFilter_HandleTypeDef *filter,
                         float value);

/**
 * @brief 用当前采样推进一次低通。
 * @param filter 调用者独占对象；未初始化时以本次输入建立历史。
 * @param input 有限采样值；单位须与历史相同。
 * @retval 滤波值；空指针返回 0。
 * @note 串行调用。更小 alpha 会增加响应滞后，不能掩盖反馈过期。
 */
float LowPassFilter_Update(LowPassFilter_HandleTypeDef *filter, float input);

#endif /* ALGORITHM_LOW_PASS_FILTER_H（防止头文件被重复包含） */
