/**
 * @file task_feed_motor_control.h
 * @brief C610 固定位置步长供弹控制。
 *
 * 鼠标左键按下沿建立一个 `FEED_MOTOR_STEP_COUNTS` 目标。目标完成后，
 * 松键结束；按住达到长按阈值且本步零电流停顿结束后才建立下一步。
 * Shift 只由运行时控制 C615，不受供弹停顿影响。
 * 仅供弹任务调用，不访问 CAN 或 PWM，不负责物理制动和卡弹检测。
 */
#ifndef TASK_FEED_MOTOR_CONTROL_H
#define TASK_FEED_MOTOR_CONTROL_H /* 防止重复包含。 */
#include "FreeRTOS.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  FEED_MOTOR_PHASE_STOP = 0, /* 无未完成目标；输出零电流。 */
  FEED_MOTOR_PHASE_STEP, /* 当前固定步长尚未完成。 */
  FEED_MOTOR_PHASE_WAIT_NEXT_STEP /* 本步完成；零电流等待步间停顿和长按阈值，松键结束。 */
} FeedMotor_PhaseTypeDef;

typedef struct {
  FeedMotor_PhaseTypeDef phase; /* 任务独占的步长阶段；初始化为 STOP。 */
  int64_t start_count; /* 本步建立时的连续逻辑 count；用于方向配置错误时的位移兜底。 */
  int64_t target_count; /* 本步目标连续逻辑 count；由步长符号决定目标方向。 */
  TickType_t press_start_tick; /* 本次按下沿的 FreeRTOS Tick；与 now_tick 同源，0 也可为真实时刻。 */
  TickType_t step_finished_tick; /* 当前步到位的 FreeRTOS Tick；仅 WAIT_NEXT_STEP 有效，停顿从这里开始。 */
  bool press_active; /* 上周期左键状态；仅任务更新，用于检测按下沿。 */
} FeedMotor_ControlTypeDef;

/**
 * @brief 初始化步长状态。
 * @param control 任务独占状态对象。
 * @retval None；空指针不操作。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control);

/**
 * @brief 推进一个非阻塞步长控制周期。
 * @param control 任务独占状态对象。
 * @param command_enabled DBUS 与 C610 反馈安全门。
 * @param button_pressed 当前鼠标左键状态。
 * @param angle_count 当前连续逻辑角度，count。
 * @param now_tick 当前 FreeRTOS Tick；用于按下沿长按计时和本步到位后的停顿计时。
 * @retval C610 协议电流；每次目标到位、步间停顿、等待或停机时为零。
 */
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, int64_t angle_count,
    TickType_t now_tick);
/**
 * @brief 返回阶段中文名称。
 * @param phase 供弹阶段枚举。
 * @retval 静态只读中文名称。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase);

#endif /* TASK_FEED_MOTOR_CONTROL_H */
