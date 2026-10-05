/**
  ******************************************************************************
  * @file    pid.h
  * @brief   与硬件无关的离散 PID 控制器。
  *
  * 本文件只处理误差、积分、微分和输出限幅，不知道电机、CAN、任务或
  * 任何具体单位。调用者必须保证 error 与 PID 参数使用同一单位体系，
  * 并传入真实的控制周期。积分抗饱和用于防止输出撞到电流/速度上限后
  * 仍持续累积，导致解除限幅时突然反向冲击。
  ******************************************************************************
  */

#ifndef ALGORITHM_PID_H
#define ALGORITHM_PID_H /* 防止 PID 接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float kp; /* 比例增益（当前误差越大，这一项给出的纠偏越大）。 */
  float ki; /* 积分增益（长期存在的小误差会逐步累积起来）。 */
  float kd; /* 微分增益（根据误差变化速度提前刹车）。 */
  float output_min; /* 输出下限（例如电流或 PWM 允许的最小值）。 */
  float output_max; /* 输出上限（防止控制器给出硬件不能承受的值）。 */
  float integral_min; /* 积分项下限（防止历史误差积累过大）。 */
  float integral_max; /* 积分项上限（与输出限幅分开控制）。 */
  float integral; /* 当前积分累加值（控制器记住的历史误差）。 */
  float previous_error; /* 上一次误差（用来计算误差变化率）。 */
  bool initialized; /* 是否已经记录过第一次误差（避免首次微分尖峰）。 */
} Pid_ControllerTypeDef;

/**
 * @brief  初始化 PID 参数并清零历史状态（让控制器从一个已知起点开始）。
 * @param  controller 控制器对象，由调用者持有（每个控制环应有自己的对象）。
 * @param  kp 比例系数（当前误差的即时纠偏强度）。
 * @param  ki 积分系数，单位与输出/(误差·秒)相符（长期误差的补偿强度）。
 * @param  kd 微分系数，当前速度环可设为 0 以避免放大测速噪声（变化太快时提前抑制）。
 * @param  output_min/output_max 输出范围，必须满足 min <= max（最终输出的安全边界）。
 * @param  integral_min/integral_max 积分项独立范围（历史误差的安全边界）。
 */
void Pid_Init(Pid_ControllerTypeDef *controller,
              float kp,
              float ki,
              float kd,
              float output_min,
              float output_max,
              float integral_min,
              float integral_max);

/**
 * @brief  清除积分和上一次误差，不改变 PID 参数（相当于忘掉旧的控制历史）。
 * @param  controller 控制器对象（不能传入空指针）。
 */
void Pid_Reset(Pid_ControllerTypeDef *controller);

/**
 * @brief  计算一个离散 PID 输出（每调用一次推进一个控制周期）。
 * @param  controller 已初始化的控制器（函数会更新积分和上次误差）。
 * @param  error 当前目标值减反馈值，单位由调用者定义（正负号决定纠偏方向）。
 * @param  dt_s 本次调用与上次调用的秒数，必须大于 0（用于积分和微分换算）。
 * @retval 限幅后的控制输出（可以直接交给下一层，但仍需遵守硬件范围）。
 *
 * @note   输出饱和且误差继续推动输出远离可用范围时，积分暂不累加；
 *         误差方向有助于离开饱和时允许积分释放（避免松开限幅后突然反冲）。
 */
float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s);

#endif /* ALGORITHM_PID_H（防止头文件被重复包含） */
