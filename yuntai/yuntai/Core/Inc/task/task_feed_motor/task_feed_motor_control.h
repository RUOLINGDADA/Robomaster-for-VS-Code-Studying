/**
 * @file task_feed_motor_control.h
 * @brief 供弹单发/连发状态机。
 *
 * 单发不使用固定角度步长。C610 以固定电流驱动，并通过连续位置停滞确认结束上弹。
 * 先让 C615 预旋到活动脉宽，再驱动 C610 上弹；上弹和停滞确认期间保持摩擦轮活动。
 * 鼠标左键长按达到阈值只记录连发意图；当前单发完成后才进入 CONTINUOUS_FEED。
 * 连发不使用停滞判定，释放或命令超时停止。
 * 本模块只处理数值和状态，不访问 CAN、PWM、DBUS 或日志。
 */
/* 调用顺序：Init→每周期 Update；运行时先按当前 phase 推进 C615，再传入同一份
 * C610 反馈快照。命令超时由 command_enabled 表示，按钮释放不能中断进行中的单发。 */
#ifndef TASK_FEED_MOTOR_CONTROL_H
#define TASK_FEED_MOTOR_CONTROL_H /* 防止重复包含。 */

#include "FreeRTOS.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  FEED_MOTOR_PHASE_STOP = 0, /* 无活动请求；C610 与 C615 停止。 */
  FEED_MOTOR_PHASE_FEED, /* C615 已预旋，单发持续驱动 C610。 */
  FEED_MOTOR_PHASE_FEED_SETTLE, /* C615 保持活动，C610 继续确认位置停滞。 */
  FEED_MOTOR_PHASE_FRIC_SPINUP, /* 首发前 C615 Ramp 到活动脉宽，C610 保持零电流。 */
  FEED_MOTOR_PHASE_SINGLE_FIRE, /* 单发 C615 已到活动脉宽，保持配置时长。 */
  FEED_MOTOR_PHASE_WAIT_RELEASE, /* 单发完成，等待松键或长按阈值。 */
  FEED_MOTOR_PHASE_CONTINUOUS_FEED /* 连发：C610 与 C615 同时持续运行。 */
} FeedMotor_PhaseTypeDef;

typedef struct {
  FeedMotor_PhaseTypeDef phase; /* 当前供弹阶段；仅供弹任务访问。 */
  TickType_t phase_start_tick; /* SINGLE_FIRE 起始 Tick；无符号差计算持续时间。 */
  int64_t last_position_count; /* 最近一次检测到明显移动的位置，逻辑电机轴 count。 */
  uint32_t stall_elapsed_ms; /* FEED_SETTLE 内连续未移动时间，ms；只在单发确认时累加。 */
  uint32_t press_elapsed_ms; /* 当前左键按下持续时间，ms；达到阈值后保留连发意图。 */
  bool press_active; /* 上一周期左键状态；用于识别 STOP→FRIC_SPINUP 的按下沿。 */
  bool continuous_requested; /* 长按已达到阈值；首发完成后切换连发。 */
} FeedMotor_ControlTypeDef;

/**
 * @brief 初始化供弹状态为停止。
 * @param control 任务独占状态对象。
 * @retval None；空指针不操作。
 * @note 仅供弹任务调用；不访问硬件。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control);

/**
 * @brief 推进一个非阻塞供弹控制周期。
 * @param control 任务独占状态对象。
 * @param command_enabled DBUS 命令和反馈安全门是否有效；false 立即停止。
 * @param button_pressed 当前合法 DBUS 帧的鼠标左键状态。
 * @param wheels_ready 两路 C615 当前是否已达到活动脉宽。
 * @param angle_count 当前 C610 连续逻辑角度，电机轴 count。
 * @param now_tick 当前 FreeRTOS Tick；用于摩擦轮保持时间。
 * @param dt_ms 本周期实际毫秒；用于按下和停滞确认计时。
 * @retval C610 协议目标电流，已按 `FEED_MOTOR_CURRENT_SIGN` 转换；非驱动阶段为 0。
 * @note 单发释放不能中断 FEED、FEED_SETTLE、FRIC_SPINUP 或 SINGLE_FIRE；命令
 *       超时仍立即停止。连发只由按键释放或命令超时停止。
 */
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, bool wheels_ready,
    int64_t angle_count, TickType_t now_tick, uint32_t dt_ms);

/**
 * @brief 判断当前阶段是否需要摩擦轮活动脉宽。
 * @param control 任务独占状态对象。
 * @retval true FEED、FEED_SETTLE、FRIC_SPINUP、SINGLE_FIRE 或 CONTINUOUS_FEED；false 其余阶段。
 */
bool FeedMotorControl_WheelsActive(const FeedMotor_ControlTypeDef *control);

/**
 * @brief 返回阶段中文名称。
 * @param phase 供弹阶段枚举。
 * @retval 静态只读中文名称；未知值返回“停止”。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase);

#endif /* TASK_FEED_MOTOR_CONTROL_H */
