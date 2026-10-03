/**
  ******************************************************************************
  * @file    ramp.h
  * @brief   与硬件无关的目标变化率限制器。
  *
  * Ramp 把一个目标值按“每秒最大变化量”逐步追踪，避免遥控输入跳变、限制目标值最多能以多快速度变化，不让指令瞬间跳变，让目标慢慢爬上去
  * 角度目标突变或保护恢复时电流一次性反向。它不关心目标是 rpm、编码器
  * 计数还是电流原始值，单位由调用者保持一致。
  ******************************************************************************
  */

#ifndef ALGORITHM_RAMP_H
#define ALGORITHM_RAMP_H /* 防止 Ramp 接口被重复包含。 */

#include <stdbool.h>

typedef struct {
  float value;// ramp当前输出值，给PID的target
  bool initialized;
} Ramp_HandleTypeDef;

/**
 * @brief  初始化 Ramp 的当前值。
 * @param  ramp 调用者持有的状态对象。
 * @param  initial_value 初始值，单位由调用者定义。
 */
void Ramp_Init(Ramp_HandleTypeDef *ramp, float initial_value);

/**
 * @brief  把 Ramp 当前值立即重置到指定值。
 * @param  ramp Ramp 状态对象。
 * @param  value 新的当前值，单位由调用者定义。
 */
 /*
  强行直接跳到某个值（不爬坡，瞬间赋值）
  使用场景：云台回零、急停、重新使能电机时，重置 ramp，避免继续慢慢爬
 */
void Ramp_Reset(Ramp_HandleTypeDef *ramp, float value);

/**
 * @brief  按最大变化率追踪目标值。
 * @param  ramp Ramp 状态。
 * @param  target_value 目标值。
 * @param  max_rate_per_s 每秒最大变化量，必须非负。（斜率）
 * @param  dt_s 本次调用间隔，单位秒。
 * @retval 本次限速后的值。限速之后，送给 PID 的目标
 */
float Ramp_Update(Ramp_HandleTypeDef *ramp,
                  float target_value,
                  float max_rate_per_s,
                  float dt_s);

#endif /* ALGORITHM_RAMP_H */
