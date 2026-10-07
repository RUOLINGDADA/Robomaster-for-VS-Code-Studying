/**
 * @file task_feed_motor_control.c
 * @brief 供弹单发/连发状态机实现。
 *
 * 单发先让 C615 预旋到活动脉宽，再用 C610 固定电流驱动，不建立角度目标。
 * 预旋完成后摩擦轮在 FEED/FEED_SETTLE 期间保持活动；连续角度在
 * `FEED_MOTOR_STALL_COUNTS` 内保持 `FEED_MOTOR_STALL_CONFIRM_MS` 后进入单发保持。
 * 长按阈值只设置连发请求，当前单发完成后才进入连发。
 * 本模块不访问硬件；运行时负责根据阶段写 C610、C615 和安全门。
 */
#include "task/task_feed_motor/task_feed_motor_control.h"
#include "task/task_feed_motor/task_feed_motor_config.h"

#include <stddef.h>
#include <stdlib.h>

_Static_assert(FEED_MOTOR_STALL_COUNTS > 0 &&
               FEED_MOTOR_STALL_CONFIRM_MS > 0U &&
               FEED_MOTOR_SINGLE_FIRE_HOLD_MS > 0U &&
               FEED_MOTOR_CONTINUOUS_PRESS_MS > 0U,
               "feed timing and stall window must be positive");
_Static_assert(FEED_MOTOR_FEED_CURRENT_RAW > 0 &&
               FEED_MOTOR_FEED_CURRENT_RAW <= FEED_MOTOR_MAX_CURRENT_RAW &&
               FEED_MOTOR_MAX_CURRENT_RAW <= 10000,
               "invalid C610 feed current");

/**
 * @brief 建立一次单发的预旋请求并清除运动确认历史。
 * @param control 任务独占状态对象。
 * @param angle_count 当前连续反馈位置，作为停滞检测初值。
 * @retval None。
 */
static void FeedMotorControl_StartSpinup(FeedMotor_ControlTypeDef *control,
                                       int64_t angle_count) {
  /* 先预旋摩擦轮；首帧不向 C610 供电，避免摩擦轮尚未建立速度就顶住弹丸。 */
  control->phase = FEED_MOTOR_PHASE_FRIC_SPINUP;
  control->last_position_count = angle_count;
  control->stall_elapsed_ms = 0U;
  control->phase_start_tick = 0U;
}

/**
 * @brief 清除状态并等待下一次鼠标按下沿。
 * @param control 任务独占状态对象。
 * @retval None。
 */
static void FeedMotorControl_Stop(FeedMotor_ControlTypeDef *control) {
  control->phase = FEED_MOTOR_PHASE_STOP;
  control->phase_start_tick = 0U;
  control->last_position_count = 0;
  control->stall_elapsed_ms = 0U;
  control->press_elapsed_ms = 0U;
  control->continuous_requested = false;
}

/**
 * @brief 累加左键持续时间并在阈值达到后锁存连发意图。
 * @param control 任务独占状态对象。
 * @param button_pressed 当前左键状态。
 * @param dt_ms 本周期毫秒。
 * @retval None。
 * @note 释放时清除按下计时，但进行中的单发由状态机继续完成。
 */
static void FeedMotorControl_UpdatePress(FeedMotor_ControlTypeDef *control,
                                         bool button_pressed,
                                         uint32_t dt_ms) {
  if (!button_pressed) {
    control->press_elapsed_ms = 0U;
    control->press_active = false;
    return;
  }
  if (control->press_elapsed_ms < FEED_MOTOR_CONTINUOUS_PRESS_MS) {
    uint32_t next_ms = control->press_elapsed_ms + dt_ms;
    if (next_ms < control->press_elapsed_ms ||
        next_ms > FEED_MOTOR_CONTINUOUS_PRESS_MS) {
      next_ms = FEED_MOTOR_CONTINUOUS_PRESS_MS;
    }
    control->press_elapsed_ms = next_ms;
  }
  if (control->press_elapsed_ms >= FEED_MOTOR_CONTINUOUS_PRESS_MS) {
    control->continuous_requested = true;
  }
  control->press_active = true;
}

/**
 * @brief 检查反馈位置是否在停滞窗口内，并推进确认计时。
 * @param control 任务独占状态对象。
 * @param angle_count 当前连续反馈位置，count。
 * @param dt_ms 本周期毫秒。
 * @retval true 已连续确认停滞；false 仍在运动或确认未完成。
 * @note 使用连续角度差，不受 0/8191 单圈回绕影响。反馈跳变超过窗口会重新计时。
 */
static bool FeedMotorControl_PositionStalled(
    FeedMotor_ControlTypeDef *control, int64_t angle_count, uint32_t dt_ms) {
  const int64_t delta = angle_count - control->last_position_count;
  if (llabs(delta) > FEED_MOTOR_STALL_COUNTS) {
    control->last_position_count = angle_count;
    control->stall_elapsed_ms = 0U;
    return false;
  }
  if (control->stall_elapsed_ms < FEED_MOTOR_STALL_CONFIRM_MS) {
    uint32_t next_ms = control->stall_elapsed_ms + dt_ms;
    if (next_ms < control->stall_elapsed_ms ||
        next_ms > FEED_MOTOR_STALL_CONFIRM_MS) {
      next_ms = FEED_MOTOR_STALL_CONFIRM_MS;
    }
    control->stall_elapsed_ms = next_ms;
  }
  return control->stall_elapsed_ms >= FEED_MOTOR_STALL_CONFIRM_MS;
}

/**
 * @brief 初始化供弹状态为停止。
 * @param control 任务独占状态对象。
 * @retval None；空指针不操作。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control) {
  if (control != NULL) {
    *control = (FeedMotor_ControlTypeDef){0};
  }
}

/**
 * @brief 推进一个非阻塞供弹周期。
 * @param control 任务独占状态对象。
 * @param command_enabled DBUS 命令和 C610 反馈安全门；false 立即停止。
 * @param button_pressed 当前合法帧的鼠标左键状态。
 * @param wheels_ready 两路 C615 是否已达到活动脉宽。
 * @param angle_count 当前连续反馈位置，逻辑电机轴 count。
 * @param now_tick 当前 FreeRTOS Tick。
 * @param dt_ms 本周期实际毫秒。
 * @retval C610 目标电流，非驱动阶段返回 0。
 * @note 单发释放不停止当前动作；只有命令超时、反馈安全门或连发释放立即停止。
 */
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, bool wheels_ready,
    int64_t angle_count, TickType_t now_tick, uint32_t dt_ms) {
  if (control == NULL || dt_ms == 0U || !command_enabled) {
    if (control != NULL) {
      FeedMotorControl_Stop(control);
    }
    return 0;
  }

  const bool press_rising = button_pressed && !control->press_active;
  FeedMotorControl_UpdatePress(control, button_pressed, dt_ms);

  /* STOP 只接受新的按下沿。释放后的单发在其它阶段继续，不会重新触发。 */
  if (control->phase == FEED_MOTOR_PHASE_STOP) {
    if (press_rising) {
      control->continuous_requested = false;
      FeedMotorControl_StartSpinup(control, angle_count);
    }
    return 0;
  }

  /* 单发阶段不因释放中断；长按只锁存请求，不能跳过预旋、上弹和停滞确认。 */
  if (control->phase == FEED_MOTOR_PHASE_FEED ||
      control->phase == FEED_MOTOR_PHASE_FEED_SETTLE) {
    if (FeedMotorControl_PositionStalled(control, angle_count, dt_ms)) {
      /* 确认完成的同周期停止 C610；C615 已预旋并持续活动，直接开始发射保持。 */
      control->phase = FEED_MOTOR_PHASE_SINGLE_FIRE;
      control->phase_start_tick = now_tick;
      return 0;
    }
    if (control->stall_elapsed_ms == 0U) {
      control->phase = FEED_MOTOR_PHASE_FEED;
    } else {
      control->phase = FEED_MOTOR_PHASE_FEED_SETTLE;
    }
    return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
  }

  if (control->phase == FEED_MOTOR_PHASE_FRIC_SPINUP) {
    if (wheels_ready) {
      /* 两路活动 PWM 已写入才允许上弹；从当前位置重新建立停滞检测基准。 */
      control->phase = FEED_MOTOR_PHASE_FEED;
      control->last_position_count = angle_count;
      control->stall_elapsed_ms = 0U;
      return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
    }
    return 0;
  }

  if (control->phase == FEED_MOTOR_PHASE_SINGLE_FIRE) {
    if ((TickType_t)(now_tick - control->phase_start_tick) >=
        pdMS_TO_TICKS(FEED_MOTOR_SINGLE_FIRE_HOLD_MS)) {
      if (control->continuous_requested && button_pressed) {
        control->phase = FEED_MOTOR_PHASE_CONTINUOUS_FEED;
        return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
      } else {
        control->phase = FEED_MOTOR_PHASE_WAIT_RELEASE;
      }
    }
    return 0;
  }

  if (control->phase == FEED_MOTOR_PHASE_WAIT_RELEASE) {
    if (control->continuous_requested && button_pressed) {
      control->phase = FEED_MOTOR_PHASE_CONTINUOUS_FEED;
      return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
    }
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
    }
    return 0;
  }

  /* 连发不调用停滞判定；只要合法左键仍按住，就同时驱动 C610 和 C615。 */
  if (control->phase == FEED_MOTOR_PHASE_CONTINUOUS_FEED) {
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
      return 0;
    }
    return (int16_t)(FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
  }

  FeedMotorControl_Stop(control);
  return 0;
}

/**
 * @brief 判断当前阶段是否需要摩擦轮活动脉宽。
 * @param control 任务独占状态对象。
 * @retval true FEED、FEED_SETTLE、FRIC_SPINUP、SINGLE_FIRE 或 CONTINUOUS_FEED；false 其余阶段。
 */
bool FeedMotorControl_WheelsActive(const FeedMotor_ControlTypeDef *control) {
  if (control == NULL) {
    return false;
  }
  return control->phase == FEED_MOTOR_PHASE_FEED ||
      control->phase == FEED_MOTOR_PHASE_FEED_SETTLE ||
      control->phase == FEED_MOTOR_PHASE_FRIC_SPINUP ||
      control->phase == FEED_MOTOR_PHASE_SINGLE_FIRE ||
      control->phase == FEED_MOTOR_PHASE_CONTINUOUS_FEED;
}

/**
 * @brief 返回阶段中文名称。
 * @param phase 供弹阶段枚举。
 * @retval 静态只读中文名称；未知值返回“停止”。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_FEED: return "供弹";
  case FEED_MOTOR_PHASE_FEED_SETTLE: return "供弹停滞确认";
  case FEED_MOTOR_PHASE_FRIC_SPINUP: return "摩擦轮预旋";
  case FEED_MOTOR_PHASE_SINGLE_FIRE: return "单发保持";
  case FEED_MOTOR_PHASE_WAIT_RELEASE: return "等待松键";
  case FEED_MOTOR_PHASE_CONTINUOUS_FEED: return "连发";
  case FEED_MOTOR_PHASE_STOP:
  default: return "停止";
  }
}
