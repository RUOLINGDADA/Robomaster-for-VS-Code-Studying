/**
 * @file task_feed_motor_control.c
 * @brief 供弹预旋、角度步进与最小间隔状态机。
 *
 * 仅供弹任务调用。STOP 取消目标；SPINUP 等待双轮目标脉宽和预旋时间。
 * ADVANCE 用角度 P 推进；INTERVAL 保持已完成目标并等待下一步。
 * 输入为同一反馈快照的逻辑连续 count 和 rpm；输出为 C610 电流原始值。
 * 误差与速度须连续满足 20 ms。目标变化后当周期重算误差，防止下一发迟一个周期。
 * 阶段时长由 ms 转 FreeRTOS Tick。本模块不访问硬件，反馈掉线由运行时检查。
 */
#include "task/task_feed_motor/task_feed_motor_control.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include <math.h>
#include <stddef.h>

/* 步长必须大于到位窗口；否则当前位置就可能满足下一步到位。
 * 电流和方向须在 C610 协议范围内，配置错误在编译期退出而不是上板猜测。 */
_Static_assert(FEED_MOTOR_STEP_COUNTS > FEED_MOTOR_WINDOW_COUNTS &&
               FEED_MOTOR_WINDOW_COUNTS >= 0, "feed step/window invalid");
_Static_assert(FEED_MOTOR_MAX_CURRENT_RAW > 0 && FEED_MOTOR_MAX_CURRENT_RAW <= 10000,
               "C610 current must stay within protocol range");
_Static_assert((FEED_MOTOR_CURRENT_SIGN == 1 || FEED_MOTOR_CURRENT_SIGN == -1) &&
               (FEED_MOTOR_FEEDBACK_SIGN == 1 || FEED_MOTOR_FEEDBACK_SIGN == -1),
               "feed signs must be +1 or -1");

/**
 * @brief 初始化控制历史为停止。
 * @param control 任务独占对象。
 * @retval None；空指针不操作。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control) {
  if (control == NULL) {
    return;
  }
  /* 零初始化对应 STOP，计步数和计时标志清零；P 把 count 误差换成 raw。
   * I/D 与积分限幅均为 0，不引入旧目标的持续积分力矩。 */
  *control = (FeedMotor_ControlTypeDef){0};
  Pid_Init(&control->position, FEED_MOTOR_POSITION_KP, 0.0f, 0.0f,
           -FEED_MOTOR_MAX_CURRENT_RAW, FEED_MOTOR_MAX_CURRENT_RAW, 0.0f, 0.0f);
}

/**
 * @brief 取消未完成步进并将目标对齐当前位置。
 * @param control 任务独占对象。
 * @param angle_count 本周期逻辑连续角度，count。
 * @retval None；清积分和阶段，输出由运行时清零。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void FeedMotorControl_Stop(FeedMotor_ControlTypeDef *control, int64_t angle_count) {
  if (control == NULL) {
    return;
  }
  /* 对齐当前逻辑反馈并取消到位确认，清除 PID 历史；保留累计 completed_steps。
   * 停止不减去已走角度，也不把未完成的一步计入完成数。（下次从当前位置重新出发。） */
  control->phase = FEED_MOTOR_PHASE_STOP;
  control->target_count = angle_count;
  control->settling = false;
  Pid_Reset(&control->position);
}

/* 任务私有：从输入基准加一份步长，并清理到位/PID 历史。
 * 首发基准是当前反馈；连续下一发基准是已完成目标，避免每发损失窗口内误差。
 * 基准使用逻辑电机轴 count，不是减速箱输出角或实发弹丸数量。 */
static void FeedMotorControl_StartStep(FeedMotor_ControlTypeDef *control,
                                      int64_t angle_count) {
  control->target_count = angle_count + FEED_MOTOR_STEP_COUNTS;
  control->phase = FEED_MOTOR_PHASE_ADVANCE;
  control->settling = false;
  Pid_Reset(&control->position);
}

/**
 * @brief 推进一次状态机并计算本周期电流。
 * @param control 已初始化的任务对象。
 * @param fire 是否请求发射且全部安全门有效。
 * @param wheels_ready 两轮到达活动脉宽；不等同于转速反馈。
 * @param angle_count 本周期逻辑连续角度，count。
 * @param speed_rpm 同份快照的逻辑速度，rpm。
 * @param now_tick 当前 FreeRTOS Tick；毫秒配置在使用点转换。
 * @param dt_ms 本周期 ms；不能传 Tick 数。
 * @retval C610 原始目标电流；未预旋完或输入非法时为 0。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
int16_t FeedMotorControl_Update(FeedMotor_ControlTypeDef *control, bool fire,
    bool wheels_ready, int64_t angle_count, int32_t speed_rpm,
    TickType_t now_tick, uint32_t dt_ms) {
  if (control == NULL) {
    return 0;
  }
  /* 松键/许可失效优先于任何阶段切换；周期为 0 或非有限增益也回 STOP。
   * 当周期直接返回 0，不能等 INTERVAL 或摩擦轮减速结束才停止拨弹。 */
  if (!fire || dt_ms == 0U || !isfinite(FEED_MOTOR_POSITION_KP) ||
      FEED_MOTOR_POSITION_KP < 0.0f) {
    FeedMotorControl_Stop(control, angle_count);
    return 0;
  }
  /* 先清理失效请求，再执行 STOP→SPINUP→ADVANCE→INTERVAL。
   * 预旋必须同时满足时长与双轮目标脉宽；PWM 到位没有实际转速反馈。 */
  if (control->phase == FEED_MOTOR_PHASE_STOP) {
    control->phase = FEED_MOTOR_PHASE_SPINUP;
    /* 只在真正进入阶段时记录起点；若每周期覆盖起点，预旋/间隔永远不会结束。 */
    control->phase_start_tick = now_tick;
    control->target_count = angle_count;
    return 0;
  }
  /* 两个条件是与关系：已过 300 ms 且双 PWM 写值到活动目标。
   * 任一不足就保持零电流；C615 没有转速反馈，不能把该条件解释为摩擦轮机械已稳。 */
  if (control->phase == FEED_MOTOR_PHASE_SPINUP) {
    if (!wheels_ready || (TickType_t)(now_tick - control->phase_start_tick) <
        pdMS_TO_TICKS(FEED_MOTOR_SPINUP_MS)) {
      return 0;
    }
    FeedMotorControl_StartStep(control, angle_count);
  }
  if (control->phase == FEED_MOTOR_PHASE_ADVANCE) {
    const float error_count = (float)(control->target_count - angle_count);
    /* 误差窗口与低转速连续满足 SETTLE_MS 才计步；任一条件离开便重新计时。
     * 只检查一次过零会把高速穿越目标误认成已完成。 */
    if (fabsf(error_count) <= FEED_MOTOR_WINDOW_COUNTS &&
        speed_rpm >= -FEED_MOTOR_READY_SPEED_RPM && speed_rpm <= FEED_MOTOR_READY_SPEED_RPM) {
      if (!control->settling) {
        control->settling = true;
        /* 首次满足窗口时开计时；持续满足期间不更新该起点，Tick 0 也是合法起点。 */
        control->settle_start_tick = now_tick;
      }
      if ((TickType_t)(now_tick - control->settle_start_tick) >= pdMS_TO_TICKS(FEED_MOTOR_SETTLE_MS)) {
        control->phase = FEED_MOTOR_PHASE_INTERVAL;
        /* 进入 INTERVAL 才记录起点；每周期覆盖会使 100 ms 间隔永远不能结束。 */
        control->phase_start_tick = now_tick;
        /* 到位确认已完成，清除本步确认标志；下一步重新建立确认起点。 */
        control->settling = false;
        /* 只在 ADVANCE→INTERVAL 时计一次；没有弹丸传感器，只能证明一次角度到位。 */
        ++control->completed_steps;
      }
    } else {
      /* 噪声或惯性使条件离开窗口，连续确认必须从下一次满足时重新开始。 */
      control->settling = false;
    }
  } else if (control->phase == FEED_MOTOR_PHASE_INTERVAL &&
             (TickType_t)(now_tick - control->phase_start_tick) >= pdMS_TO_TICKS(FEED_MOTOR_SHOT_INTERVAL_MS)) {
    /* 以已完成目标为下一步起点，避免每发少走到位窗口的计数。 */
    FeedMotorControl_StartStep(control, control->target_count);
  }
  /* 状态可能在上面的间隔分支切换；此处必须重新读取目标，不能沿用旧目标误差。 */
  const float error_count = (float)(control->target_count - angle_count);
  /* count 误差乘 P 输出 raw，dt_ms 换为 s；PID 先限幅，再按独立 current_sign 转物理方向。
   * INTERVAL 仍计算保持电流，不是零电流等待；STEP 目标切换也在本周期立即作用。 */
  return (int16_t)(Pid_Update(&control->position, error_count,
                              (float)dt_ms * 0.001f) * FEED_MOTOR_CURRENT_SIGN);
}

/**
 * @brief 返回阶段中文名称。
 * @param phase 阶段枚举。
 * @retval 静态只读字符串；未知枚举返回“停止”，不能释放或改写。
 * @note 无可变状态，可并发查询；本工程用于供弹任务日志。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_SPINUP: return "预旋";
  case FEED_MOTOR_PHASE_ADVANCE: return "角度步进";
  case FEED_MOTOR_PHASE_INTERVAL: return "发射间隔";
  case FEED_MOTOR_PHASE_STOP:
  default: return "停止";
  }
}
