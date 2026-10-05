/**
  ******************************************************************************
  * @file    gimbal_axis.h
  * @brief   Yaw/Pitch 共用的 GM6020 角度闭环运行时。
  *
  * 每个轴独立保存电机句柄、控制器和标定范围。软件限位只过滤继续向外
  * 的命令，角度环、速度环、Ramp 和抗重力补偿在限位处继续运行；动态
  * 堵转锁存和固定限位电流不属于本模块。
  ******************************************************************************
  */
#ifndef GIMBAL_AXIS_H
#define GIMBAL_AXIS_H /* 防止通用云台轴接口被重复包含。 */

#include "app/gimbal/gimbal_control.h"
#include "bsp/gm6020/gm6020.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  GIMBAL_AXIS_PHASE_INIT = 0,
  GIMBAL_AXIS_PHASE_WAIT_FEEDBACK,
  GIMBAL_AXIS_PHASE_HOLD_POSITION,
  GIMBAL_AXIS_PHASE_ACTIVE,
  GIMBAL_AXIS_PHASE_LIMIT_HOLD,
  GIMBAL_AXIS_PHASE_FEEDBACK_LOST
} GimbalAxis_PhaseTypeDef;

typedef enum {
  GIMBAL_AXIS_REASON_NONE = 0,
  GIMBAL_AXIS_REASON_SOFT_MIN,
  GIMBAL_AXIS_REASON_SOFT_MAX,
  GIMBAL_AXIS_REASON_FEEDBACK_LOST,
  GIMBAL_AXIS_REASON_CALIBRATION_INVALID,
  GIMBAL_AXIS_REASON_TARGET_INVALID,
  GIMBAL_AXIS_REASON_CONTROL_INVALID,
  GIMBAL_AXIS_REASON_INPUT_HOLD
} GimbalAxis_ReasonTypeDef;

typedef struct {
  Gm6020_ConfigTypeDef motor; /* CAN、ID、方向和反馈超时；机械范围来自本轴标定。 */
  Gm6020_CalibrationTypeDef calibration; /* 本轴独立中心、最小和最大连续角度。 */
  GimbalControl_ConfigTypeDef control; /* 角度环、速度环、Ramp 和抗重力参数。 */
} GimbalAxis_ConfigTypeDef;

typedef struct {
  Gm6020_SnapshotTypeDef snapshot; /* 本周期唯一快照，日志不能重新读取驱动。 */
  GimbalControl_OutputTypeDef control; /* 控制器原始结果，包含边界过滤后的目标。 */
  GimbalAxis_PhaseTypeDef phase; /* 本周期运行阶段。 */
  GimbalAxis_ReasonTypeDef reason; /* 本周期诊断原因。 */
  Gm6020_LimitTypeDef limit; /* 只读的当前几何边界状态。 */
  uint32_t feedback_age_ms; /* 反馈年龄，单位 HAL ms。 */
  int16_t applied_current_raw; /* 经过电机方向转换后准备发送的电流。 */
  bool snapshot_valid; /* 快照复制是否成功。 */
  bool output_enabled; /* 本周期是否允许发送非零电流。 */
  bool can_submitted; /* 聚合帧是否交给 HAL 邮箱。 */
} GimbalAxis_OutputTypeDef;

typedef struct {
  Gm6020_HandleTypeDef motor; /* 本轴独占的 GM6020 句柄，ISR 只更新反馈。 */
  GimbalAxis_ConfigTypeDef config; /* 初始化时复制的本轴配置。 */
  GimbalControl_HandleTypeDef controller; /* 本轴角度/速度控制历史。 */
  GimbalAxis_OutputTypeDef cycle; /* 本周期控制、反馈和发送诊断。 */
  bool initialized; /* 驱动是否已注册。 */
  bool calibration_valid; /* min<center<max 且标定有效。 */
  bool parameters_valid; /* 增益、限幅和滤波参数是否合法。 */
  bool controller_initialized; /* 是否已用首帧反馈初始化控制器。 */
  bool feedback_lost; /* 曾经掉线，恢复首周期先清理控制历史。 */
} GimbalAxis_HandleTypeDef;

/** @brief 初始化一个独立云台轴；仅任务上下文调用。 */
bool GimbalAxis_Init(GimbalAxis_HandleTypeDef *axis,
                     const GimbalAxis_ConfigTypeDef *config);

/** @brief 执行一个相对速度命令周期；无效/过期命令按零速度保持当前位置。 */
void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms);

/** @brief 显式发送零电流并清除输出许可；反馈掉线和初始化失败使用。 */
void GimbalAxis_RunDisabledCycle(GimbalAxis_HandleTypeDef *axis,
                                 uint32_t now_ms);

/** @brief 执行固定目标角度周期；与正式模式共用控制和软件限位。 */
void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms);

/** @brief 发送本轴零电流，返回值仅表示 HAL 是否接受聚合帧。 */
bool GimbalAxis_SendZero(GimbalAxis_HandleTypeDef *axis);

/** @brief 非阻塞输出本周期诊断；只读取 cycle 中的同一份快照。 */
bool GimbalAxis_TryLog(GimbalAxis_HandleTypeDef *axis,
                       const char *label, int16_t command_permille);

/** @brief 返回阶段中文名称。 */
const char *GimbalAxis_PhaseName(GimbalAxis_PhaseTypeDef phase);

/** @brief 返回诊断原因中文名称。 */
const char *GimbalAxis_ReasonName(GimbalAxis_ReasonTypeDef reason);

#endif /* GIMBAL_AXIS_H（防止轴运行时接口被重复包含） */
