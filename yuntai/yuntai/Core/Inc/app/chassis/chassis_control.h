/**
 * @file chassis_control.h
 * @brief 四轮底盘的目标混控、键盘曲线和独立速度 PID。
 *
 * 仅所属底盘任务串行调用；输入为值副本和真实 dt_s，不访问 DBUS、HAL 或 CAN。
 * 轮序按 ID1右前、ID2左前、ID3左后、ID4右后；输出仍为逻辑方向，安装符号由运行时处理。
 */
#ifndef APP_CHASSIS_CONTROL_H
#define APP_CHASSIS_CONTROL_H /* 防止重复包含。 */
#include "algorithm/pid/pid.h"
#include <stdint.h>

#define CHASSIS_WHEEL_COUNT 4U /* 底盘轮数，个；数组顺序为电调 ID1~4。 */

typedef enum {
  CHASSIS_MODE_STOP = 0, /* 零电流，清历史；上电和命令掉线默认模式。 */
  CHASSIS_MODE_STICK, /* CH2/CH3 平移，不使用旋转通道。 */
  CHASSIS_MODE_KEYBOARD /* W/S、A/D、Q/E；输入位图按 DJI DBUS 协议。 */
} Chassis_ModeTypeDef;

typedef struct {
  float max_speed_rpm; /* 逻辑转子转速上限，rpm，有限正值；轮轴速度须除减速比。 */
  float kp; /* 速度比例增益，raw/rpm，有限非负值。 */
  float ki; /* 速度积分增益，raw/(rpm*s)，有限非负值。 */
  float kd; /* 速度微分增益，raw*s/rpm，有限非负值；默认 0。 */
  float integral_limit_raw; /* 积分项绝对限幅，raw，非负且不大于电流限幅。 */
  float integral_error_scale; /* 变速积分系数，1/rpm，非负；0 关闭加权。 */
  float current_limit_raw; /* 实际速度环输出限幅，raw，范围 (0,16384]。 */
  int16_t stick_deadband_raw; /* 摇杆中心死区，raw，范围 [0,660)。 */
  float ramp_t1_s; /* 键盘曲线直线段结束时间，s，有限正值。 */
  float ramp_t3_s; /* 键盘曲线完成时间，s，须大于 t1。 */
  float ramp_linear_ratio; /* t1 时速度比例，无量纲，范围 (0,1)。 */
  float ramp_log_shape; /* 对数形状系数，无量纲，有限正值。 */
} ChassisControl_ConfigTypeDef;

typedef struct {
  float value_rpm; /* 当前曲线输出，rpm；所属底盘任务独占。 */
  float start_rpm; /* 本次过渡起点，rpm。 */
  float target_rpm; /* 当前过渡终点，rpm；反向先以 0 为终点。 */
  float elapsed_s; /* 过渡已用时，s，由实际 dt_s 推进。 */
  bool active; /* 当前正在过渡；完成周期返回终点而不跨过零速。 */
} ChassisControl_RampAxisTypeDef;

typedef struct {
  float speed_rpm; /* 已做安装方向变换的逻辑转子 rpm，须为有限值。 */
  bool online; /* 此轮本周期快照合法且新鲜；false 只清该轮 PID。 */
} ChassisControl_FeedbackTypeDef;

typedef struct {
  float target_speed_rpm[CHASSIS_WHEEL_COUNT]; /* 混控限幅后的逻辑转子 rpm。 */
  int16_t current_raw[CHASSIS_WHEEL_COUNT]; /* 限流后的逻辑电流 raw，尚未变换安装方向。 */
  float axis_speed_rpm[3]; /* 混控前 Vx/Vy/Wz 等效转子 rpm，便于观察键盘曲线。 */
} ChassisControl_OutputTypeDef;

typedef struct {
  ChassisControl_ConfigTypeDef config; /* Init 值复制配置；所属任务只读。 */
  Pid_ControllerTypeDef speed_pid[CHASSIS_WHEEL_COUNT]; /* 每轮独立积分、微分历史。 */
  ChassisControl_RampAxisTypeDef ramp[3]; /* Vx/Vy/Wz 各自曲线状态，Reset 清零。 */
  bool ready; /* 配置有效且四个 PID 已初始化；false 时 Update 返回零。 */
} ChassisControl_HandleTypeDef;

/**
 * @brief 校验并复制控制参数，初始化四个速度 PID 与曲线。
 * @param control 所属任务独占的控制状态。
 * @param config 参数副本来源，不保存指针。
 * @retval true 完成；false 参数非法，控制器置未就绪。
 * @note 任务或主机调用；不进行 CAN、DBUS 或 RTOS 操作。
 */
bool ChassisControl_Init(ChassisControl_HandleTypeDef *control, const ChassisControl_ConfigTypeDef *config);
/**
 * @brief 清除控制历史，不改变配置。
 * @param control 所属控制器；NULL 不操作。
 * @retval None。
 * @note 同一任务串行调用；模式切换及通信恢复前使用。
 */
void ChassisControl_Reset(ChassisControl_HandleTypeDef *control);
/**
 * @brief 更新一次四轮控制；离线轮只清自己的 PID。
 * @param control 已初始化控制器，任务独占。
 * @param mode 停机、摇杆或键盘模式。
 * @param vx_raw 左右摇杆中心化 raw，仅 STICK 使用，按 ±660 限幅。
 * @param vy_raw 前后摇杆中心化 raw，仅 STICK 使用。
 * @param keyboard DJI 键盘位图，仅 KEYBOARD 使用，不包含模式判定。
 * @param feedback 本周期四轮一致反馈副本，按 ID1~4 排列。
 * @param dt_s 实际控制间隔，s，有限正值。
 * @param output 本周期输出，失败或停机时清零。
 * @retval true 更新完成；false 配置、周期、模式或参数非法，清历史。
 * @note 不发送；零速度在运动模式运行速度闭环制动，STOP 直接零电流。
 */
bool ChassisControl_Update(ChassisControl_HandleTypeDef *control, Chassis_ModeTypeDef mode,
    int16_t vx_raw, int16_t vy_raw, uint16_t keyboard,
    const ChassisControl_FeedbackTypeDef feedback[CHASSIS_WHEEL_COUNT], float dt_s,
    ChassisControl_OutputTypeDef *output);
#endif /* APP_CHASSIS_CONTROL_H */
