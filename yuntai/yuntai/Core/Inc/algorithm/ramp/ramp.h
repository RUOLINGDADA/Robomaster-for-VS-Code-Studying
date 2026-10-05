/**
  ******************************************************************************
  * @file    ramp.h
  * @brief   与硬件无关的目标变化率限制器。
  *
  * 通俗理解：目标值就像油门，Ramp 不允许它一下子从小变到大，
  * 而是按“每秒最多改变多少”逐步跟上，避免电流或速度突然跳变。
  *
  * Ramp 把一个目标值按“每秒最大变化量”逐步追踪，避免遥控输入跳变或保护恢复时电流一次性反向。
  * 如果直接把目标值交给电机，控制周期中的一个采样就可能造成大电流阶跃；它不关心目标是 rpm、编码器
  * 计数还是电流原始值，单位由调用者保持一致。
  ******************************************************************************
  */

#ifndef ALGORITHM_RAMP_H
#define ALGORITHM_RAMP_H /* 防止 Ramp 接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float value; /* 当前已经输出的值（送给 PID 的是这个平滑后的数值）。 */
  bool initialized; /* 是否已经有起始值（第一次更新前需要先建立起点）。 */
} Ramp_HandleTypeDef;

/**
 * @brief  初始化 Ramp 的当前值。
 * @param  ramp 调用者持有的状态对象（保存当前平滑值）。
 * @param  initial_value 初始值，单位由调用者定义（通常设为当前实际输出）。
 */
void Ramp_Init(Ramp_HandleTypeDef *ramp, float initial_value);

/**
 * @brief  把 Ramp 当前值立即重置到指定值。
 * @param  ramp Ramp 状态对象（调用者独占，不能与别的控制器共用）。
 * @param  value 新的当前值，单位由调用者定义（不会经过爬坡过程）。
 */
/* 立即跳到指定值，不经过爬坡过程（云台回零、急停或重新使能时清除旧的变化趋势）。 */
void Ramp_Reset(Ramp_HandleTypeDef *ramp, float value);

/**
 * @brief  按最大变化率追踪目标值（每次只走允许的最大一步）。
 * @param  ramp Ramp 状态（函数会更新其中的当前值）。
 * @param  target_value 目标值（最终想要到达的数值）。
 * @param  max_rate_per_s 每秒最大变化量，必须非负（变化率单位与目标值一致）。
 * @param  dt_s 本次调用间隔，单位秒（决定这一次最多走多远）。
 * @retval 本次限速后的值（可直接作为控制器下一阶段的目标）。
 */
float Ramp_Update(Ramp_HandleTypeDef *ramp,
                  float target_value,
                  float max_rate_per_s,
                  float dt_s);

#endif /* ALGORITHM_RAMP_H（防止头文件被重复包含） */
