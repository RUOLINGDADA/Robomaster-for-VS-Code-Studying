/**
 * @file gimbal_control.h
 * @brief 云台角度环、速度 PID、重力补偿与电流 Ramp。
 *
 * 角度使用连续 count，速度使用 rpm，输出使用 GM6020 电流原始值。
 * 每轴由所属任务独占实例。输入反馈必须来自同一份 CAN 快照。
 * 本模块只计算输出，不发送 CAN，不检查反馈年龄或配置合法性。
 * 运行时先检查安全门；边界只过滤向外命令，反向命令仍可离开边界。
 */

#ifndef GIMBAL_CONTROL_H
#define GIMBAL_CONTROL_H /* 防止通用云台控制接口被重复包含（避免控制结构和函数声明重复）。 */

#include "algorithm/filter/low_pass_filter.h"
#include "algorithm/gravity_compensation/gravity_compensation.h"
#include "algorithm/pid/pid.h"
#include "algorithm/ramp/ramp.h"
#include "bsp/gm6020/gm6020.h"
#include "app/gimbal/gimbal_command.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int32_t center_angle_raw; /* 标定中心连续角度，单位编码器计数（位置目标围绕它定义）。 */
  int32_t min_angle_raw; /* 允许的连续角度下限，单位编码器计数（软件安全边界）。 */
  int32_t max_angle_raw; /* 允许的连续角度上限，单位编码器计数（必须大于下限）。 */
  float max_command_speed_rpm; /* 相对命令满量程对应速度，单位 rpm（输入满杆最多有多快）。 */
  float max_speed_target_rpm; /* 角度环输出速度上限，单位 rpm（位置误差不能让速度超过此值）。 */
  float position_kp_rpm_per_raw; /* 角度环比例增益，单位 rpm/计数（角度差越大，速度目标越大）。 */
  float position_ki_rpm_per_raw_s; /* 角度环积分增益，单位 rpm/(计数·s)（补偿持续静差，必须配合积分限幅）。 */
  float position_integral_limit_rpm; /* 角度环积分项绝对值上限，单位 rpm（防止重力静差长期积累成大目标速度）。 */
  float position_kd_rpm_s_per_raw; /* 角度环微分增益，单位 rpm·s/计数（误差变化提供阻尼，默认可为 0）。 */
  float speed_kp_current_per_rpm; /* 速度比例增益，单位电流原始值/rpm（速度差的即时纠偏）。 */
  float speed_ki_current_per_rpm_s; /* 速度积分增益，单位原始值/(rpm·s)（补偿长期速度误差）。 */
  float speed_kd_current_s_per_rpm; /* 速度微分增益，单位原始值·s/rpm，默认 0（差分会放大测速噪声和目标突变）。 */
  float speed_integral_limit_raw; /* 速度积分项绝对值上限，单位原始值，允许 0（限制历史补偿，不替代总电流限幅）。 */
  float velocity_feedforward_gain; /* 命令速度前馈系数，无量纲，默认 1（直接提供运动速度，固定目标零速度时不起作用）。 */
  float max_current_raw; /* 速度环输出电流上限，单位 GM6020 原始值（控制器的力矩上限）。 */
  float current_slew_raw_per_s; /* 电流 Ramp 上限，单位原始值/秒（限制电流改变速度）。 */
  float speed_filter_alpha; /* 速度低通滤波权重，范围 0~1（越小越平滑但响应越慢）。 */
  GravityCompensation_ConfigTypeDef gravity_compensation; /* 独立算法配置；不改变目标位置。 */
} GimbalControl_ConfigTypeDef;

typedef struct {
  Pid_ControllerTypeDef speed_pid; /* 速度 PID 控制器历史；D 默认为 0，沿用 PI（每轴独立保存积分和上一周期误差）。 */
  Ramp_HandleTypeDef current_ramp; /* 电流目标变化率限制状态（防止电流突变）。 */
  LowPassFilter_HandleTypeDef speed_filter; /* 实测速度滤波状态（先去掉测速毛刺）。 */
  GravityCompensation_HandleTypeDef gravity_compensation; /* 重力算法状态；每轴独占。 */
  GimbalControl_ConfigTypeDef config; /* 初始化时复制的控制参数（不保存外部配置指针）。 */
  float target_angle_raw; /* 当前保持目标，单位连续编码器计数（松手后云台要停在这里）。 */
  float position_integral_rpm; /* 位置环积分历史，单位 rpm（每轴独立，反馈恢复时清零，不改最后目标）。 */
  float target_speed_rpm; /* 上一次计算出的速度目标，单位 rpm（角度环给速度环的目标）。 */
  int32_t center_angle_raw; /* 控制器保存的标定中心，单位连续计数（用于诊断和重置）。 */
  bool initialized; /* true 表示 PID、Ramp 和滤波器已初始化（可以开始计算）。 */
} GimbalControl_HandleTypeDef;

typedef struct {
  int32_t target_angle_raw; /* 本周期目标连续角度，单位编码器计数（位置环希望到达的位置）。 */
  int32_t angle_error_raw; /* 目标减实际角度误差，单位编码器计数（正负决定纠偏方向）。 */
  float target_speed_rpm; /* 本周期速度目标，单位 rpm（送入速度环）。 */
  int16_t speed_loop_current_raw; /* 位置/速度环输出，未叠加重力补偿，单位原始电流。 */
  int16_t gravity_compensation_current_raw; /* 本周期重力补偿分量，单位原始电流。 */
  int16_t target_current_raw; /* 本周期电流目标，GM6020 原始值（驱动反馈新鲜度门之前的控制结果）。 */
  bool at_min_limit; /* true 表示反馈已到达/越过软件最小边界（遥控外向意图会被过滤）。 */
  bool at_max_limit; /* true 表示反馈已到达/越过软件最大边界（遥控外向意图会被过滤）。 */
} GimbalControl_OutputTypeDef;

/**
 * @brief  初始化角度/速度级联控制器及其滤波、Ramp 状态（建立控制历史的起点）。
 * @param  controller 调用者持有的控制器对象（每个云台轴独占一个）。
 * @param  config 控制增益、范围、速度和电流变化率参数（单位必须与反馈一致）。
 * @param  initial_angle_raw 第一次有效反馈的连续角度，单位原始计数（作为初始保持位置）。
 * @note   正式轴运行时先校验增益非负/有限、积分上限不超过总电流、滤波权重在 (0,1]（直接调用者也须满足这些前提）。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_Init(GimbalControl_HandleTypeDef *controller,
                     const GimbalControl_ConfigTypeDef *config,
                     int32_t initial_angle_raw);

/**
 * @brief  清除控制器动态历史并保留最后的目标位置。
 * @param  controller 已初始化的通用云台控制器（不能与另一轴共用）。
 * @note   用于反馈恢复；不会改写 target_angle_raw，避免丢失最后一次有效控制位置。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_ResetHistory(GimbalControl_HandleTypeDef *controller);

/**
 * @brief  设置一个固定保持目标，不改变 PID、Ramp 和滤波历史（测试角度环时使用）。
 * @param  controller 已初始化的通用云台控制器（调用者独占）。
 * @param  target_angle_raw 目标连续角度，函数会钳位到配置范围（越界不会继续向外驱动）。
 * @retval true 设置成功；false 参数为空或控制器尚未初始化。
 * @note   适用于硬件角度保持调参；正式相对速度模式仍通过速度命令改变目标（两种模式复用同一控制器）。
 */
bool GimbalControl_SetTargetAngle(GimbalControl_HandleTypeDef *controller,
                               int32_t target_angle_raw);

/**
 * @brief  运行位置 PID、速度 PID、重力补偿与统一电流 Ramp。
 * @param  controller 通用云台控制器（函数会更新 PID、滤波和 Ramp 历史）。
 * @param  feedback 同一次 CAN 快照中的反馈（角度和速度必须来自同一帧）。
 * @param  command 已完成超时检查的遥控或固定目标命令（固定目标标记避免零速度被当成回中）。
 * @param  dt_s 控制周期，单位秒（用于把 rpm 换算成这段时间内的角度变化）。
 * @param  output 输出的目标角度、速度和电流（调用者随后交给驱动）。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                       const Gm6020_FeedbackTypeDef *feedback,
                       const Gimbal_CommandTypeDef *command,
                       float dt_s,
                       GimbalControl_OutputTypeDef *output);

#endif /* GIMBAL_CONTROL_H（防止控制接口被重复包含） */
