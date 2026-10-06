/**
 * @file pid.h
 * @brief 离散 PID 与抗积分饱和。
 *
 * 调用者定义误差和输出单位，并传入真实周期 dt_s。
 * 实例由所属控制任务独占。不访问 CAN、PWM 或 RTOS 对象。
 * 输出饱和时禁止积分继续向外累加，防止恢复后过冲。
 */

#ifndef ALGORITHM_PID_H
#define ALGORITHM_PID_H /* 防止 PID 接口被重复包含（避免类型和函数声明重复）。 */

#include <stdbool.h>

typedef struct {
  float kp; /* 比例增益，输出单位/误差单位；Init 写入，所属控制任务读取。 */
  float ki; /* 积分增益，输出单位/(误差单位·s)；增加会加快静差补偿。 */
  float kd; /* 微分增益，输出单位·s/误差单位；增加会放大采样噪声。 */
  float output_min; /* 输出下限，单位由调用者定义；必须不大于 output_max。 */
  float output_max; /* 输出上限，与下限同单位；上层须核对硬件安全范围。 */
  float integral_min; /* 积分项下限，与输出同单位；不替代总输出限幅。 */
  float integral_max; /* 积分项上限，与输出同单位；必须不小于 integral_min。 */
  float integral; /* 已包含 ki 的积分项，与输出同单位；所属任务更新，Reset 清零。 */
  float previous_error; /* 上次误差，与本次 error 同单位；Update 写入，Reset 清零。 */
  bool initialized; /* 已有可差分的误差历史；Reset 后 false，首帧避免微分尖峰。 */
} Pid_ControllerTypeDef;

/**
 * @brief 复制 PID 参数并清除控制历史。
 * @param controller 调用者独占对象；空指针不操作。
 * @param kp 比例增益，输出单位/误差单位。
 * @param ki 积分增益，输出单位/(误差单位·s)。
 * @param kd 微分增益，输出单位·s/误差单位。
 * @param output_min 输出下限；须不大于 output_max。
 * @param output_max 输出上限；与下一级输入单位相同。
 * @param integral_min 积分项下限；须不大于 integral_max。
 * @param integral_max 积分项上限；与输出同单位。
 * @retval None；参数由上层保证有限且范围有效。
 * @note 串行调用。每个控制环独占实例，不能共用积分历史。
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
 * @brief 清除积分与微分历史，保留参数。
 * @param controller 调用者独占对象；空指针不操作。
 * @retval None；下次 Update 避免首帧微分尖峰。
 * @note 串行调用。反馈恢复时重置，防止旧积分推动电机。
 */
void Pid_Reset(Pid_ControllerTypeDef *controller);

/**
 * @brief 计算一次限幅后的离散 PID 输出。
 * @param controller 已配置且独占的控制器。
 * @param error 目标减反馈，单位与增益一致；须为有限值。
 * @param dt_s 实际周期，s；须为有限正值。
 * @retval 限幅输出；空指针或非正周期返回 0。
 * @note 串行调用。饱和时只接受有助于离开饱和的积分，防止解除限幅后反冲。
 */
float Pid_Update(Pid_ControllerTypeDef *controller, float error, float dt_s);

#endif /* ALGORITHM_PID_H（防止头文件被重复包含） */
