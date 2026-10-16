/**
 * @file task_feed_motor_control.c
 * @brief C610 固定位置步长状态机。
 *
 * 目标采用连续角度 count，避免单圈回绕破坏大小关系。每次左键按下沿
 * 将当前反馈加上有符号步长；到目标的周期清零，长按经过步间停顿后才建立下一步。
 * 仅任务调用；位移兜底防止反向反馈让目标不可达，不访问硬件或检测卡弹。
 */
#include "task/task_feed_motor/task_feed_motor_control.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include <stddef.h>

_Static_assert(FEED_MOTOR_STEP_COUNTS != 0, "feed step must be non-zero");
_Static_assert(pdMS_TO_TICKS(FEED_MOTOR_CONTINUOUS_PRESS_MS) > 0U,
               "feed hold threshold must be at least one tick");
_Static_assert(pdMS_TO_TICKS(FEED_MOTOR_STEP_PAUSE_MS) > 0U,
               "feed pause must be at least one tick");
_Static_assert(FEED_MOTOR_FEED_CURRENT_RAW > 0 &&
               FEED_MOTOR_FEED_CURRENT_RAW <= FEED_MOTOR_MAX_CURRENT_RAW &&
               FEED_MOTOR_MAX_CURRENT_RAW <= 10000,
               "invalid C610 feed current");

/* 计算两个连续 count 的无符号距离，避免 INT64_MIN 的取负溢出。 */
static uint64_t FeedMotorControl_Distance(int64_t left, int64_t right) {
  /* 先比较有符号大小，再用无符号减法；这样差值本身不会溢出。 */
  return left >= right ? (uint64_t)left - (uint64_t)right
                       : (uint64_t)right - (uint64_t)left;
}

/* 目标方向按符号比较；位移兜底防止反馈方向配反时电机永远不停。 */
static bool FeedMotorControl_TargetReached(const FeedMotor_ControlTypeDef *control,
                                           int64_t angle_count) {
  const bool target_reached = FEED_MOTOR_STEP_COUNTS > 0
      ? angle_count >= control->target_count
      : angle_count <= control->target_count;
  const bool distance_reached =
      FeedMotorControl_Distance(angle_count, control->start_count) >=
      FeedMotorControl_Distance(control->target_count, control->start_count);
  return target_reached || distance_reached;
}

/* 清除当前目标。释放只影响下一步，已经进行的步长仍由调用方继续推进。 */
static void FeedMotorControl_Stop(FeedMotor_ControlTypeDef *control) {
  control->phase = FEED_MOTOR_PHASE_STOP;
  control->start_count = 0;
  control->target_count = 0;
  control->press_start_tick = 0U;
  control->step_finished_tick = 0U;
}

/* 以本周期反馈建立目标；先检查加法边界，越界则停止，不能让溢出目标触发错误步长。 */
static bool FeedMotorControl_StartStep(FeedMotor_ControlTypeDef *control,
                                       int64_t angle_count) {
  const int64_t step_count = FEED_MOTOR_STEP_COUNTS;
  if ((step_count > 0 && angle_count > INT64_MAX - step_count) ||
      (step_count < 0 && angle_count < INT64_MIN - step_count)) {
    FeedMotorControl_Stop(control);
    return false;
  }
  control->start_count = angle_count;
  control->target_count = angle_count + step_count;
  control->phase = FEED_MOTOR_PHASE_STEP;
  return true;
}

/** @brief 初始化步长状态；仅任务上下文调用。 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control) {
  if (control != NULL) {
    *control = (FeedMotor_ControlTypeDef){0};
  }
}

/** @brief 推进一步；命令或反馈安全门关闭时清零并停止。 */
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control,
    bool command_enabled, bool button_pressed, int64_t angle_count,
    TickType_t now_tick) {
  /* 先检查安全门，再记录按下沿时刻；不能把按下前的空闲周期计入长按。
   * 所有阶段使用同源 Tick 差，阈值在比较处从 ms 换算，支持 Tick 回绕和不同内核频率。 */
  if (control == NULL || !command_enabled) {
    if (control != NULL) {
      FeedMotorControl_Stop(control);
      control->press_active = button_pressed;
    }
    return 0;
  }
  const bool press_rising = button_pressed && !control->press_active;
  if (press_rising) {
    control->press_start_tick = now_tick;
  }
  control->press_active = button_pressed;
  const bool long_press = button_pressed &&
      (TickType_t)(now_tick - control->press_start_tick) >=
          pdMS_TO_TICKS(FEED_MOTOR_CONTINUOUS_PRESS_MS);

  if (control->phase == FEED_MOTOR_PHASE_WAIT_NEXT_STEP) {
    if (!button_pressed) {
      FeedMotorControl_Stop(control);
      return 0;
    }
    /* 停顿从本步到位计时，不能从按下沿或上一目标建立时计时。
     * 达到长按阈值也不能跳过零电流阶段，否则目标逐个完成却始终狂转。 */
    const bool pause_finished =
        (TickType_t)(now_tick - control->step_finished_tick) >=
            pdMS_TO_TICKS(FEED_MOTOR_STEP_PAUSE_MS);
    if (!long_press || !pause_finished ||
        !FeedMotorControl_StartStep(control, angle_count)) {
      return 0;
    }
  }
  if (control->phase == FEED_MOTOR_PHASE_STOP) {
    if (!press_rising) {
      return 0;
    }
    if (!FeedMotorControl_StartStep(control, angle_count)) {
      return 0;
    }
  }
  if (FeedMotorControl_TargetReached(control, angle_count)) {
    if (button_pressed) {
      control->phase = FEED_MOTOR_PHASE_WAIT_NEXT_STEP;
      control->step_finished_tick = now_tick;
    } else {
      FeedMotorControl_Stop(control);
    }
    /* 到位周期一律结束并发送零，下一步只能在后续周期等待完整停顿后建立。 */
    return 0;
  }
  /* 步长决定逻辑方向，CURRENT_SIGN 将逻辑方向映射到协议方向。
   * 忽略步长符号会让负目标始终配正电流，只能依赖位移兜底错误结束。 */
  return (int16_t)((FEED_MOTOR_STEP_COUNTS > 0 ? 1 : -1) *
      FEED_MOTOR_FEED_CURRENT_RAW * FEED_MOTOR_CURRENT_SIGN);
}

/** @brief 返回日志使用的阶段名称。 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_STOP: return "停止";
  case FEED_MOTOR_PHASE_STEP: return "步长供弹";
  case FEED_MOTOR_PHASE_WAIT_NEXT_STEP: return "步间停顿";
  default: return "未知阶段";
  }
}
