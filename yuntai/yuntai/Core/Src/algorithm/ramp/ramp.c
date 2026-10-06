/**
 * @file ramp.c
 * @brief 按变化率追踪目标的非阻塞 Ramp。
 *
 * 目标、历史值和变化率使用同一单位体系；周期使用 s。
 * 实例由所属任务独占。不访问外设，不检查反馈或输入许可。
 * 错误周期会改变输出斜率。故障停机应直接重置，不能等待 Ramp。
 */

#include "algorithm/ramp/ramp.h"

#include <stddef.h>

/**
 * @brief 建立 Ramp 起点。
 * @param ramp 调用者独占对象；空指针不操作。
 * @param initial_value 有限初值；单位由调用者定义。
 * @retval None；保存初值并标记已初始化。
 * @note 串行调用；不写硬件输出。
 */
void Ramp_Init(Ramp_HandleTypeDef *ramp, float initial_value) {
  if (ramp == NULL) {
    return;
  }
  ramp->value = initial_value;
  ramp->initialized = true;
}

/**
 * @brief 立即重建 Ramp 起点。
 * @param ramp 调用者独占对象；空指针不操作。
 * @param value 有限新值；与目标同单位。
 * @retval None；跳过斜坡，清除旧输出趋势。
 * @note 串行调用。上层故障停机可重置历史，再写硬件停止值。
 */
void Ramp_Reset(Ramp_HandleTypeDef *ramp, float value) {
  Ramp_Init(ramp, value);
}

/**
 * @brief 按每秒最大变化率追踪目标。
 * @param ramp 调用者独占对象；未初始化时直接以目标建立起点。
 * @param target_value 有限目标；与历史同单位。
 * @param max_rate_per_s 有限非负变化率，目标单位/s。
 * @param dt_s 有限正周期，s；不能传 ms 或 Tick。
 * @retval 本周期限速值；空指针、非正周期或负变化率返回 0。
 * @note 串行调用。允许步长为 rate×dt；错误单位会使输出跳变。
 */
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
