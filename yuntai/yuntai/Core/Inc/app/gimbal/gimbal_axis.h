/**
 * @file gimbal_axis.h
 * @brief GM6020 单轴闭环、反馈安全门与周期诊断。
 *
 * Yaw/Pitch 各自独占实例。所属任务每周期只复制一份反馈。
 * CAN1 ISR 更新驱动；任务组合控制、输出许可、聚合帧和 UART 日志。
 * 命令失效按零速度保持；反馈或参数失效清零。恢复时重建控制历史。
 * 本模块不创建任务，不锁存堵转，不执行机械急停。
 */
#ifndef GIMBAL_AXIS_H
#define GIMBAL_AXIS_H /* 防止通用云台轴接口被重复包含。 */

#include "app/gimbal/gimbal_control.h"
#include "bsp/gm6020/gm6020.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  GIMBAL_AXIS_PHASE_INIT = 0, /* 尚未建立轴运行状态。 */
  GIMBAL_AXIS_PHASE_WAIT_FEEDBACK, /* 等待有效首帧或有效参数；零输出。 */
  GIMBAL_AXIS_PHASE_HOLD_POSITION, /* 零相对速度下保持位置。 */
  GIMBAL_AXIS_PHASE_ACTIVE, /* 按相对速度移动目标。 */
  GIMBAL_AXIS_PHASE_LIMIT_HOLD, /* 边界过滤向外命令，继续闭环保持。 */
  GIMBAL_AXIS_PHASE_FEEDBACK_LOST /* 反馈过期；零输出并等待恢复。 */
} GimbalAxis_PhaseTypeDef;

typedef enum {
  GIMBAL_AXIS_REASON_NONE = 0, /* 本周期无附加诊断。 */
  GIMBAL_AXIS_REASON_SOFT_MIN, /* 反馈在软件下界或之外。 */
  GIMBAL_AXIS_REASON_SOFT_MAX, /* 反馈在软件上界或之外。 */
  GIMBAL_AXIS_REASON_FEEDBACK_LOST, /* 反馈无效或超过 HAL ms 期限。 */
  GIMBAL_AXIS_REASON_CALIBRATION_INVALID, /* 标定未确认或 min<center<max 不成立。 */
  GIMBAL_AXIS_REASON_TARGET_INVALID, /* 固定目标无效或越界。 */
  GIMBAL_AXIS_REASON_CONTROL_INVALID, /* 控制参数、输入或周期非法。 */
  GIMBAL_AXIS_REASON_INPUT_HOLD /* 零速度或输入失效；保持位置。 */
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

/**
 * @brief 复制配置并注册独立 GM6020 轴。
 * @param axis 持久轴对象；每个任务独占一个。
 * @param config CAN、标定和控制参数；先完成板级 CAN 初始化。
 * @retval true 已注册；false 空指针或驱动注册失败。标定/增益许可另存于对象。
 * @note 仅所属任务首次调用。注册成功不代表已收到反馈或允许闭环。
 */
bool GimbalAxis_Init(GimbalAxis_HandleTypeDef *axis,
                     const GimbalAxis_ConfigTypeDef *config);

/**
 * @brief 按相对速度推进一个正式闭环周期。
 * @param axis 已注册轴对象。
 * @param command 速度 ±1000‰；空、失效或越界时按零速度保持。
 * @param now_ms HAL_GetTick 当前 ms；与反馈时间戳同源。
 * @param dt_ms 控制周期 ms；必须大于 0。
 * @retval None；周期输出写入 axis->cycle，反馈或参数失效时发零电流。
 * @note 仅所属任务调用。每周期只取一份反馈快照，不等待 CAN 发送完成。
 */
void GimbalAxis_RunCycle(GimbalAxis_HandleTypeDef *axis,
                         const Gimbal_CommandTypeDef *command,
                         uint32_t now_ms, uint32_t dt_ms);

/**
 * @brief 使用正式控制器保持固定角度。
 * @param axis 已注册轴对象。
 * @param target_angle_raw 标定范围内的连续 count；越界发零电流。
 * @param now_ms HAL 当前 ms；用于反馈年龄。
 * @param dt_ms 控制周期 ms；必须大于 0。
 * @retval None；周期结果写入 axis->cycle。
 * @note 仅所属任务调用。固定目标和相对速度模式同周期不能同时执行。
 */
void GimbalAxis_RunFixedTarget(GimbalAxis_HandleTypeDef *axis,
                               int32_t target_angle_raw,
                               uint32_t now_ms, uint32_t dt_ms);

/**
 * @brief 关闭本轴输出许可并提交零电流聚合帧。
 * @param axis 已注册轴对象。
 * @retval true HAL 已接受帧；false 句柄无效、邮箱忙或 HAL 失败。
 * @note 仅所属任务调用。失败仍保留零目标；提交成功不证明电调已停止。
 */
bool GimbalAxis_SendZero(GimbalAxis_HandleTypeDef *axis);

/**
 * @brief 取得阶段中文名称。
 * @param phase 当前阶段枚举；未知值按初始化描述。
 * @retval 静态只读字符串；不能修改或释放。
 * @note 纯查询，无硬件或共享状态操作。
 */
const char *GimbalAxis_PhaseName(GimbalAxis_PhaseTypeDef phase);

/**
 * @brief 取得诊断原因中文名称。
 * @param reason 当前原因枚举；未知值按无原因描述。
 * @retval 静态只读字符串；不能修改或释放。
 * @note 纯查询，无硬件或共享状态操作。
 */
const char *GimbalAxis_ReasonName(GimbalAxis_ReasonTypeDef reason);

#endif /* GIMBAL_AXIS_H（防止轴运行时接口被重复包含） */
