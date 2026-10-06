/**
 * @file task_feed_motor_control.h
 * @brief 供弹预旋、角度步进与最小间隔状态机。
 *
 * 仅供弹任务调用。STOP 取消目标；SPINUP 等待双轮目标脉宽和预旋时间。
 * ADVANCE 用角度 P 推进；INTERVAL 保持已完成目标并等待下一步。
 * 输入为同一反馈快照的逻辑连续 count 和 rpm；输出为 C610 电流原始值。
 * 误差与速度须连续满足 20 ms。目标变化后当周期重算误差，防止下一发迟一个周期。
 * 阶段时长由 ms 转 FreeRTOS Tick。本模块不访问硬件，反馈掉线由运行时检查。
 */
/* 调用顺序：Init→每周期 Update；许可失效时 Update 内调用 Stop。
 * 上层须先校验反馈新鲜度，使用同份角度/速度快照；本模块返回 raw，不直接触碰 CAN/PWM。 */
#ifndef TASK_FEED_MOTOR_CONTROL_H
#define TASK_FEED_MOTOR_CONTROL_H /* 防止重复包含。 */
#include "FreeRTOS.h"
#include "algorithm/pid/pid.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  FEED_MOTOR_PHASE_STOP = 0, /* 无发射请求或安全门无效；零电流。 */
  FEED_MOTOR_PHASE_SPINUP, /* 摩擦轮预旋；零电流。 */
  FEED_MOTOR_PHASE_ADVANCE, /* 向一个新目标步进；角度 P 输出。 */
  FEED_MOTOR_PHASE_INTERVAL /* 到位后等待；角度 P 保持该目标。 */
} FeedMotor_PhaseTypeDef;

typedef struct {
  FeedMotor_PhaseTypeDef phase; /* 本周期阶段；由供弹任务独占。 */
  int64_t target_count; /* 逻辑电机轴连续 count；Stop 对齐当前反馈，StartStep 增加 STEP，不使用单圈回绕值。 */
  TickType_t phase_start_tick; /* SPINUP/INTERVAL 进入时的 FreeRTOS Tick；仅切换时更新，无符号差算持续时间。 */
  TickType_t settle_start_tick; /* 首次满足误差/速度窗口的 FreeRTOS Tick；仅 settling=true 时有效，0 Tick 也合法。 */
  bool settling; /* 连续确认正在进行；Stop/StartStep、到位完成或任一条件失效时清 false，下次重新计时。 */
  Pid_ControllerTypeDef position; /* count→C610 raw 的 P 控制器；I/D=0，停止与每个新步长清除历史，任务独占。 */
  uint32_t completed_steps; /* ADVANCE→INTERVAL 时累加；Init 清零，Stop 保留，uint32_t 会回绕；不是实发弹数。 */
} FeedMotor_ControlTypeDef;

/**
 * @brief 初始化控制历史为停止。
 * @param control 任务独占对象。
 * @retval None；空指针不操作。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void FeedMotorControl_Init(FeedMotor_ControlTypeDef *control);
/**
 * @brief 取消未完成步进并将目标对齐当前位置。
 * @param control 任务独占对象。
 * @param angle_count 本周期逻辑连续角度，count。
 * @retval None；清积分和阶段，输出由运行时清零。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void FeedMotorControl_Stop(FeedMotor_ControlTypeDef *control, int64_t angle_count);
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
    TickType_t now_tick, uint32_t dt_ms);
/**
 * @brief 返回阶段中文名称。
 * @param phase 阶段枚举。
 * @retval 静态只读字符串；未知枚举返回“停止”，不能释放或改写。
 * @note 无可变状态，可并发查询；本工程用于供弹任务日志。
 */
const char *FeedMotorControl_PhaseName(FeedMotor_PhaseTypeDef phase);
#endif /* TASK_FEED_MOTOR_CONTROL_H */
