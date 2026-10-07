/**
 * @file test_c610_m2006_angle_step.h
 * @brief C610/M2006 非阻塞只读手动角度步长测量。
 *
 * 测试只读取 C610/M2006 反馈，不给电机任何驱动电流，不读取 DBUS，也不启动
 * C615。首帧反馈建立角度基准，之后持续输出当前连续角度、相对基准增量、
 * 相邻反馈增量和转速。操作者手动转动拨弹机构，根据日志确认反馈方向和连续角度展开；
 * 当前正式供弹不再使用固定步长。
 * 仅任务上下文调用；CAN ISR 只更新正式驱动反馈。
 */
#ifndef TEST_C610_M2006_ANGLE_STEP_H
#define TEST_C610_M2006_ANGLE_STEP_H /* 防止 C610 角度步长测试接口重复包含。 */

#include "bsp/c610_m2006/c610_m2006.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool enabled; /* 测试许可；false 时清零电流并重置角度基准。 */
  int16_t feedback_sign; /* 原始角度/速度到逻辑坐标的符号，必须为 +1/-1。 */
  uint32_t log_period_ms; /* 当前角度日志最小间隔，HAL ms。 */
} C610_M2006_AngleStepConfigTypeDef;

typedef enum {
  C610_M2006_ANGLE_STEP_WAIT_FEEDBACK = 0, /* 等待第一帧新鲜反馈，输出零。 */
  C610_M2006_ANGLE_STEP_MONITOR /* 手动转动监测，始终输出零电流。 */
} C610_M2006_AngleStepPhaseTypeDef;

typedef struct {
  C610_M2006_AngleStepPhaseTypeDef phase; /* 测试阶段；只由供弹任务访问。 */
  int64_t reference_count; /* 首帧逻辑连续角度，手动测量的零点。 */
  int64_t previous_count; /* 上一个任务周期采样角度，用于计算相邻采样增量。 */
  uint32_t last_log_ms; /* 最近成功日志时间，HAL ms。 */
  int64_t delta_from_reference; /* 当前角度相对手动测量零点的增量，逻辑 count。 */
  int64_t delta_since_previous; /* 相邻任务采样的角度增量，逻辑 count；不是一次拨弹的步长。 */
  bool initialized; /* 已建立首帧角度基准。 */
  bool log_started; /* 至少成功提交过一条日志。 */
} C610_M2006_AngleStepStateTypeDef;

/**
 * @brief 执行一个手动角度步长测量周期。
 * @param motor 已注册的 C610/M2006 句柄；反馈和发送复用正式驱动。
 * @param state 任务独占的持久测试状态。
 * @param config task_feed_motor 组装的测试参数；调用期间保持有效。
 * @param now_ms HAL_GetTick() 当前毫秒。
 * @param dt_ms 本周期真实毫秒；只读测量保留该参数以兼容任务周期接口，不参与控制。
 * @retval None。函数始终提交零电流；参数无效、反馈过期或测试关闭时清零状态。
 * @note 只做一个非阻塞周期；不能在 CAN ISR 调用或等待电调响应。操作者必须
 *       手动转动拨弹机构，日志中的 delta_from_reference 即为相对基准的 count。
 */
void C610_M2006_AngleStep_RunCycle(
    C610_M2006_HandleTypeDef *motor,
    C610_M2006_AngleStepStateTypeDef *state,
    const C610_M2006_AngleStepConfigTypeDef *config,
    uint32_t now_ms,
    uint32_t dt_ms);

#endif /* TEST_C610_M2006_ANGLE_STEP_H */
