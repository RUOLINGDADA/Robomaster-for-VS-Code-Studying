/**
  ******************************************************************************
  * @file    gimbal_axis.h
  * @brief   轴无关的正式 GM6020 控制运行时。
  *
  * 每轴独立初始化；相对速度命令与固定目标走同一条角度 P、速度 PI、
  * 电流 Ramp、软件边界和堵转路径。模块不读取具体任务的命令邮箱。
  ******************************************************************************
  */
#ifndef GIMBAL_AXIS_H
#define GIMBAL_AXIS_H /* 防止通用云台轴运行时重复包含。 */

#include "app/gimbal/gimbal_control.h"
#include "app/gimbal/gimbal_protection.h"
#include "bsp/gm6020/gm6020.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  GIMBAL_AXIS_PHASE_INIT = 0,
  GIMBAL_AXIS_PHASE_WAIT_FEEDBACK,
  GIMBAL_AXIS_PHASE_HOLD_POSITION,
  GIMBAL_AXIS_PHASE_ACTIVE,
  GIMBAL_AXIS_PHASE_LIMIT_HOLD,
  GIMBAL_AXIS_PHASE_STALL_HOLD,
  GIMBAL_AXIS_PHASE_FEEDBACK_LOST
} GimbalAxis_PhaseTypeDef;

typedef struct {
  Gm6020_ConfigTypeDef motor; /* CAN 实例、ID 和超时；角度参考由标定自动生成。 */
  Gm6020_CalibrationTypeDef calibration; /* 本轴实测中心和有安全余量的边界。 */
  GimbalControl_ConfigTypeDef control; /* 正式角度环/速度 PI/Ramp 增益及限幅。 */
  GimbalProtection_ConfigTypeDef protection; /* 正式堵转阈值、宽限与恢复距离。 */
} GimbalAxis_ConfigTypeDef;

typedef struct {
  Gm6020_SnapshotTypeDef snapshot; /* 唯一反馈快照；日志不得重新读取驱动。 */
  GimbalControl_OutputTypeDef control; /* 本周期保护前的计算结果，含误差。 */
  GimbalAxis_PhaseTypeDef phase; /* 本周期结束时阶段，供日志观察。 */
  GimbalProtection_ReasonTypeDef reason; /* 本周期阻止输出的原因。 */
  Gm6020_LimitTypeDef limit; /* 本周期处理后的驱动软件边界原因。 */
  uint32_t feedback_age_ms; /* 快照反馈年龄，单位 HAL ms。 */
  uint32_t candidate_ms; /* 堵转候选连续时间，单位 ms。 */
  int8_t blocked_direction; /* 本周期堵转方向，事件日志保存触发值。 */
  int16_t applied_current_raw; /* 本周期经过保护后缓存的电流命令原始值。 */
  bool snapshot_valid; /* 复制成功；离线快照也可用于“反馈丢失”诊断。 */
  bool output_enabled; /* 本周期经保护后是否允许输出，不表示 CAN 已到达电调。 */
  bool can_submitted; /* 聚合控制帧成功交给 HAL 邮箱，不代表总线应答成功。 */
} GimbalAxis_OutputTypeDef;

typedef struct {
  Gm6020_HandleTypeDef motor; /* 当前轴独占的 GM6020 句柄，ISR 更新反馈。 */
  GimbalAxis_ConfigTypeDef config; /* 本轴初始化时复制的正式参数。 */
  GimbalControl_HandleTypeDef controller; /* 本轴角度/速度控制历史。 */
  LowPassFilter_HandleTypeDef current_filter; /* 堵转判断的本轴反馈电流滤波。 */
  GimbalProtection_HandleTypeDef protection; /* 保护状态，不与另一轴共享。 */
  GimbalAxis_OutputTypeDef cycle; /* 本周期控制、保护、输出和反馈一致诊断。 */
  GimbalAxis_OutputTypeDef event; /* 堵转进入/解除时保存的完整周期现场。 */
  const char *event_name; /* 静态中文事件名；串口忙时保留现场等待重试。 */
  bool initialized; /* true 表示驱动已注册。 */
  bool calibration_valid; /* 标定标志与 min<center<max 检查全部通过。 */
  bool parameters_valid; /* 增益、限幅、滤波和保护参数合法。 */
  bool controller_initialized; /* 首帧到达后已初始化控制器。 */
  bool feedback_lost; /* 曾掉线；恢复后的首个周期必须零输出。 */
} GimbalAxis_HandleTypeDef;

/**
 * @brief  从一份实测配置初始化一个独立云台轴。
 * @param  axis 调用方独占的运行时对象。
 * @param  config 电机、标定、增益与保护参数；函数复制，不保存指针。
 * @retval true 驱动注册成功；false 参数空或 ID 注册失败。
 * @note   无效标定允许只读调参，但任何闭环模式保持零输出；只在任务调用。
 */
bool GimbalAxis_Init(GimbalAxis_HandleTypeDef *axis,
                     const GimbalAxis_ConfigTypeDef *config);
/**
 * @brief  正式相对速度命令执行一个周期，零命令保持最后角度。
 * @param  axis 该轴运行时。
 * @param  command 已由轴命令适配层处理超时的相对速度命令。
 * @param  now_ms HAL_GetTick() 当前 ms。
 * @param  dt_ms 控制周期 ms，大于零。
 * @note   只在任务调用；每周期只取一次快照，不调用任务命令邮箱。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms);
/**
 * @brief  正式运行时保持固定目标，供两轴硬件调参共用。
 * @param  axis 本轴独占的运行时。
 * @param  target_angle_raw 相对标定坐标的连续角度；越界直接零输出。
 * @param  now_ms HAL 当前 ms。
 * @param  dt_ms 周期 ms。
 * @note   不伪造零速度遥控命令；固定目标堵转必须手动移开才能恢复。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms);
/**
 * @brief  禁止本轴输出并尝试发送零电流，保留另一轴命令槽位。
 * @param  axis 已初始化的本轴对象。
 * @retval true CAN 帧成功提交；false 无效对象或邮箱繁忙。
 */
bool GimbalAxis_SendZero(GimbalAxis_HandleTypeDef *axis);
/**
 * @brief  非阻塞打印本轴事件或当前周期诊断，所有反馈来自 cycle/event。
 * @param  axis 本轴对象，不会重新读取 GM6020。
 * @param  label 日志前缀，例如“水平轴角度测试”。
 * @param  command_permille 当前相对速度意图，用于诊断。
 * @retval true 日志已接受；false USART 忙，调用者稍后重试。
 */
bool GimbalAxis_TryLog(GimbalAxis_HandleTypeDef *axis,
                       const char *label, int16_t command_permille);
/**
 * @brief  中文阶段名。
 * @param  phase 周期阶段枚举。
 * @retval 静态文本，无需释放。
 */
const char *GimbalAxis_PhaseName(GimbalAxis_PhaseTypeDef phase);
#endif /* GIMBAL_AXIS_H */
