/**
  ******************************************************************************
  * @file    gimbal_protection.h
  * @brief   Yaw/Pitch 共用的软件边界、堵转确认和恢复策略。
  *
  * 纯计算模块，不访问 CAN、串口或 FreeRTOS。任务提供同一份反馈快照的
  * 速度、电流和位置。固定目标的零命令不能解释为操作者松手。
  ******************************************************************************
  */
#ifndef GIMBAL_PROTECTION_H
#define GIMBAL_PROTECTION_H /* 防止通用云台保护接口重复包含。 */
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  GIMBAL_PROTECTION_NONE = 0,
  GIMBAL_PROTECTION_SOFT_MIN,
  GIMBAL_PROTECTION_SOFT_MAX,
  GIMBAL_PROTECTION_STALL_HOLD,
  GIMBAL_PROTECTION_FEEDBACK_LOST,
  GIMBAL_PROTECTION_CALIBRATION_INVALID,
  GIMBAL_PROTECTION_TARGET_INVALID,
  GIMBAL_PROTECTION_CONTROL_INVALID
} GimbalProtection_ReasonTypeDef;

typedef struct {
  uint32_t startup_grace_ms; /* 输出从关闭到开启后的堵转观察宽限，单位 ms。 */
  uint32_t reversal_grace_ms; /* 输入或输出换向后的观察宽限，单位 ms。 */
  uint32_t stall_time_ms; /* 堵转证据连续成立时间，单位 ms。 */
  uint32_t release_confirm_ms; /* 恢复证据连续成立时间，单位 ms。 */
  int16_t stall_speed_threshold_rpm; /* 判为低速的绝对转速上限，单位 rpm。 */
  uint16_t stall_command_current_threshold_raw; /* 堵转目标电流绝对值下限。 */
  uint16_t stall_feedback_current_threshold_raw; /* 堵转反馈电流绝对值下限。 */
  uint16_t stall_position_delta_raw; /* 候选窗口内位移上限，单位编码器计数。 */
  uint16_t release_feedback_current_threshold_raw; /* 恢复前反馈电流绝对值上限。 */
  uint16_t release_position_delta_raw; /* 固定目标手动反向移开距离，单位计数。 */
  int32_t min_angle_raw; /* 连续角度下限，初始化时从本轴标定复制。 */
  int32_t max_angle_raw; /* 连续角度上限，初始化时从本轴标定复制。 */
} GimbalProtection_ConfigTypeDef;

typedef struct {
  bool feedback_online; /* 来自当前反馈快照的新鲜度判断。 */
  bool output_enabled; /* 本周期控制器请求非零输出；不是上一周期的旧句柄状态。 */
  bool fixed_target; /* true：固定角度意图；false：真实相对速度命令。 */
  int32_t angle_total_raw; /* 同一次反馈快照的连续角度，单位计数。 */
  int16_t filtered_speed_rpm; /* 当前快照滤波后的有符号速度，单位 rpm。 */
  int16_t filtered_current_raw; /* 当前快照滤波后的有符号反馈电流原始值。 */
  int16_t requested_current_raw; /* 本周期控制器计算出的电流原始值。 */
  int16_t command_velocity_permille; /* 相对模式 -1000~1000；固定模式为误差方向。 */
  uint32_t now_ms; /* 与 CAN 反馈同源的 HAL 毫秒时间。 */
} GimbalProtection_InputTypeDef;

typedef struct {
  bool allow_output; /* false 时必须立即走零输出路径。 */
  bool entered; /* 本周期刚确认堵转，供事件日志保存触发现场。 */
  bool cleared; /* 本周期刚解除保持；本周期仍禁止输出。 */
  GimbalProtection_ReasonTypeDef reason; /* 本周期保护原因。 */
  int8_t blocked_direction; /* -1/0/+1；堵转触发时电流方向。 */
} GimbalProtection_OutputTypeDef;

typedef struct {
  GimbalProtection_ConfigTypeDef config; /* 该轴独立保护参数副本。 */
  GimbalProtection_ReasonTypeDef reason; /* 锁存原因，掉线不能清除已确认堵转。 */
  int8_t blocked_direction; /* 堵转时的驱动方向，-1 或 +1。 */
  int8_t last_drive_direction; /* 上次非零电流方向，用于换向宽限。 */
  int8_t last_command_direction; /* 上次输入方向，用于提前识别换向。 */
  uint32_t guard_start_ms; /* 启动/换向宽限起点，单位 HAL ms。 */
  uint32_t candidate_start_ms; /* 连续堵转候选起点，单位 HAL ms。 */
  uint32_t release_start_ms; /* 恢复证据连续成立起点，单位 HAL ms。 */
  uint32_t guard_duration_ms; /* 当前宽限时长，单位 ms。 */
  int32_t candidate_angle_raw; /* 候选起点的连续角度，用于位移检测。 */
  int32_t stall_trigger_angle_raw; /* 确认堵转时的位置，恢复判据的参考点。 */
  bool candidate_active; /* 单独标记计时有效性，不能把时间戳 0 当作未开始。 */
  bool release_active; /* 单独标记恢复计时有效性，兼容 ms 回绕。 */
  bool output_was_enabled; /* 上次输出许可，用于识别实际重新启动。 */
  bool stall_requires_manual_release; /* 触发时固定目标模式，之后零命令不能解除。 */
} GimbalProtection_HandleTypeDef;

/**
 * @brief  清零一个轴的保护状态并复制参数。
 * @param  protection 任务独占的保护对象。
 * @param  config 本轴堵转、范围和恢复参数。
 * @note   纯计算，不操作电机；只在对象初始化时调用。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalProtection_Init(GimbalProtection_HandleTypeDef *protection,
                            const GimbalProtection_ConfigTypeDef *config);
/**
 * @brief  清除候选和恢复计时，保留已锁存的堵转或软件限位。
 * @param  protection 本轴保护对象。
 * @note   CAN 掉线时使用，不能把反馈中断当作堵转已经消失。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalProtection_OnFeedbackLost(GimbalProtection_HandleTypeDef *protection);
/**
 * @brief  根据一份一致输入更新保护，函数不发送 CAN。
 * @param  protection 任务独占的保护状态。
 * @param  input 本周期快照与输入意图。
 * @param  output 本周期是否允许电流以及进入/解除事件。
 * @note   调用者必须执行输出结果；无效参数默认禁止输出。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalProtection_Update(GimbalProtection_HandleTypeDef *protection,
                              const GimbalProtection_InputTypeDef *input,
                              GimbalProtection_OutputTypeDef *output);
/**
 * @brief  返回稳定的中文保护原因。
 * @param  reason 原因枚举。
 * @retval 静态字符串，无需释放。
 */
const char *GimbalProtection_ReasonName(GimbalProtection_ReasonTypeDef reason);
#endif /* GIMBAL_PROTECTION_H */
