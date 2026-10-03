/**
  ******************************************************************************
  * @file    gimbal_control.h
  * @brief   通用云台相对角度保持的角度环、速度环和电流输出。
  *
  * 输入是相对转速命令：持续推动时目标角度按速度移动，松手后目标角度
  * 停在最后位置。角度 P 环给出速度目标，速度 PI 环给出 GM6020 电流原始
  * 值。这里不访问 CAN、FreeRTOS 或串口，便于在主机上用合成反馈测试。
  ******************************************************************************
  */

#ifndef GIMBAL_CONTROL_H
#define GIMBAL_CONTROL_H /* 防止通用云台控制接口被重复包含。 */

#include "algorithm/filter/low_pass_filter.h"
#include "algorithm/pid/pid.h"
#include "algorithm/ramp/ramp.h"
#include "bsp/gm6020/gm6020.h"
#include "app/gimbal/gimbal_command.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int32_t center_angle_raw; /* 标定中心连续角度，单位编码器计数。 */
  int32_t min_angle_raw; /* 允许的连续角度下限，单位编码器计数。 */
  int32_t max_angle_raw; /* 允许的连续角度上限，单位编码器计数。 */
  float max_command_speed_rpm; /* 相对命令满量程对应速度，单位 rpm。 */
  float max_speed_target_rpm; /* 角度环输出速度上限，单位 rpm。 */
  float position_kp_rpm_per_raw; /* 角度环比例增益，单位 rpm/计数。 */
  float speed_kp_current_per_rpm; /* 速度比例增益，单位电流原始值/rpm。 */
  float speed_ki_current_per_rpm_s; /* 速度积分增益，单位原始值/(rpm·s)。 */
  float max_current_raw; /* 速度环输出电流上限，单位 GM6020 原始值。 */
  float current_slew_raw_per_s; /* 电流 Ramp 上限，单位原始值/秒。 */
  float speed_filter_alpha; /* 速度低通滤波权重，范围 0~1。 */
} GimbalControl_ConfigTypeDef;

typedef struct {
  Pid_ControllerTypeDef speed_pid; /* 速度 PI 控制器的积分和限幅状态。 */
  Ramp_HandleTypeDef current_ramp; /* 电流目标变化率限制状态。 */
  LowPassFilter_HandleTypeDef speed_filter; /* 实测速度滤波状态。 */
  GimbalControl_ConfigTypeDef config; /* 初始化时复制的控制参数。 */
  float target_angle_raw; /* 当前保持目标，单位连续编码器计数。 */
  float target_speed_rpm; /* 上一次计算出的速度目标，单位 rpm。 */
  int32_t center_angle_raw; /* 控制器保存的标定中心，单位连续计数。 */
  bool initialized; /* true 表示 PID、Ramp 和滤波器已初始化。 */
} GimbalControl_HandleTypeDef;

typedef struct {
  int32_t target_angle_raw; /* 本周期目标连续角度，单位编码器计数。 */
  int32_t angle_error_raw; /* 目标减实际角度误差，单位编码器计数。 */
  float target_speed_rpm; /* 本周期速度目标，单位 rpm。 */
  int16_t target_current_raw; /* 本周期电流目标，GM6020 原始值。 */
  bool at_min_limit; /* true 表示当前命令试图越过软件最小边界。 */
  bool at_max_limit; /* true 表示当前命令试图越过软件最大边界。 */
} GimbalControl_OutputTypeDef;

/**
 * @brief  初始化角度/速度级联控制器及其滤波、Ramp 状态。
 * @param  controller 调用者持有的控制器对象。
 * @param  config 控制增益、范围、速度和电流变化率参数。
 * @param  initial_angle_raw 第一次有效反馈的连续角度，单位原始计数。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_Init(GimbalControl_HandleTypeDef *controller,
                     const GimbalControl_ConfigTypeDef *config,
                     int32_t initial_angle_raw);

/**
 * @brief  清除控制器历史量并把当前反馈角度设为新的保持目标。
 * @param  controller 已初始化的 通用云台控制器。
 * @param  angle_raw 当前连续编码器角度，单位为原始计数。
 * @note   用于堵转释放和反馈恢复，避免旧目标角度突然追赶。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_ResetToAngle(GimbalControl_HandleTypeDef *controller,
                             int32_t angle_raw);

/**
 * @brief  设置一个固定保持目标，不改变 PID、Ramp 和滤波历史。
 * @param  controller 已初始化的 通用云台控制器。
 * @param  target_angle_raw 目标连续角度，函数会钳位到配置范围。
 * @retval true 设置成功；false 参数为空或控制器尚未初始化。
 * @note   适用于硬件角度保持调参；正式相对速度模式仍通过速度命令改变目标。
 */
bool GimbalControl_SetTargetAngle(GimbalControl_HandleTypeDef *controller,
                               int32_t target_angle_raw);

/**
 * @brief  运行一次角度/速度级联控制。
 * @param  controller 通用云台控制器。
 * @param  feedback 同一次 CAN 快照中的反馈。
 * @param  command 已完成超时检查的遥控命令。
 * @param  dt_s 控制周期，单位秒。
 * @param  hold_output true 时冻结目标并输出零电流。
 * @param  output 输出的目标角度、速度和电流。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                       const Gm6020_FeedbackTypeDef *feedback,
                       const Gimbal_CommandTypeDef *command,
                       float dt_s,
                       bool hold_output,
                       GimbalControl_OutputTypeDef *output);

#endif /* GIMBAL_CONTROL_H */
