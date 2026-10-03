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
#define ALGORITHM_PID_H /* 防止 PID 接口被重复包含。 */

#include <stdbool.h>

typedef struct {
  float kp;
  float ki;
  float kd;
  float output_min; //输出限幅，电机PWM/电流限幅
  float output_max;
  float integral_min; //积分项限幅
  float integral_max;
  float integral; //积分累加和
  float previous_error; //上一次误差，用来算微分项
  bool initialized;
} Pid_ControllerTypeDef;

/**
 * @brief  初始化 PID 参数并清零历史状态。
 * @param  controller 控制器对象，由调用者持有。
 * @param  kp 比例系数。
 * @param  ki 积分系数，单位与输出/(误差·秒)相符。
 * @param  kd 微分系数，当前速度环可设为 0 以避免放大测速噪声。
 * @param  output_min/output_max 输出范围，必须满足 min <= max。
 * @param  integral_min/integral_max 积分项独立范围。
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
 * @brief  清除积分和上一次误差，不改变 PID 参数。
 * @param  controller 控制器对象。
 */
void Pid_Reset(Pid_ControllerTypeDef *controller);

/**
 * @brief  计算一个离散 PID 输出。
 * @param  controller 已初始化的控制器。
 * @param  error 当前目标值减反馈值，单位由调用者定义。
 * @param  dt_s 本次调用与上次调用的秒数，必须大于 0。
 * @retval 限幅后的控制输出。
 *
 * @note   输出饱和且误差继续推动输出远离可用范围时，积分暂不累加；
 *         误差方向有助于离开饱和时允许积分释放。
 */
float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s);

#endif /* ALGORITHM_PID_H */
