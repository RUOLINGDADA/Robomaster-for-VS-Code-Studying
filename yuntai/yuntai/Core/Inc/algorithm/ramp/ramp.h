/**
 * @file ramp.h
 * @brief 按变化率追踪目标的非阻塞 Ramp。
 *
 * 目标、历史值和变化率使用同一单位体系；周期使用 s。
 * 实例由所属任务独占。不访问外设，不检查反馈或输入许可。
 * 错误周期会改变输出斜率。故障停机应直接重置，不能等待 Ramp。
 */

#ifndef ALGORITHM_RAMP_H
#define ALGORITHM_RAMP_H /* 防止 Ramp 接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float value; /* 变化率受限的当前值，与 target_value 同单位；所属任务更新。 */
  bool initialized; /* 已建立起点；false 时首次 Update 直接采用目标，不能用于渐进启动。 */
} Ramp_HandleTypeDef;

/**
 * @brief 建立 Ramp 起点。
 * @param ramp 调用者独占对象；空指针不操作。
 * @param initial_value 有限初值；单位由调用者定义。
 * @retval None；保存初值并标记已初始化。
 * @note 串行调用；不写硬件输出。
 */
void Ramp_Init(Ramp_HandleTypeDef *ramp, float initial_value);

/**
 * @brief 立即重建 Ramp 起点。
 * @param ramp 调用者独占对象；空指针不操作。
 * @param value 有限新值；与目标同单位。
 * @retval None；跳过斜坡，清除旧输出趋势。
 * @note 串行调用。上层故障停机可重置历史，再写硬件停止值。
 */
void Ramp_Reset(Ramp_HandleTypeDef *ramp, float value);

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
                  float dt_s);

#endif /* ALGORITHM_RAMP_H（防止头文件被重复包含） */
