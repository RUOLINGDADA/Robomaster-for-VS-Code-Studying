/**
  ******************************************************************************
  * @file    test_gm6020_angle_loop.h
  * @brief   不绑定 Yaw/Pitch 的 GM6020 固定目标角度环硬件调参接口。
  ******************************************************************************
  */

#ifndef TEST_GM6020_ANGLE_LOOP_H
#define TEST_GM6020_ANGLE_LOOP_H /* 防止 GM6020 角度测试接口重复包含。 */

#include "app/gimbal/gimbal_axis.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  const char *axis_name; /* 日志轴名称，不能使用共享可变字符串。 */
  float target_angle_deg; /* 相对本轴中位的固定目标，单位度。 */
  uint32_t log_period_ms; /* 诊断日志最短间隔，单位 ms。 */
  uint32_t last_log_ms; /* 本实例上一次日志时间，单位 HAL 毫秒。 */
  bool log_started; /* true 表示该实例至少成功输出一条日志。 */
  bool target_warning_logged; /* 防止无效配置提示刷屏。 */
} Gm6020_TestAngleLoopTypeDef;

/**
 * @brief  执行一次固定目标角度环硬件调参。
 * @param  motor 该轴 GM6020 句柄，用于读取本周期一致快照。
 * @param  calibration 该轴中心与边界配置。
 * @param  axis 正式通用角度控制实例，不能与其它轴共享。
 * @param  test 该轴独立测试状态和目标角度参数。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。
 * @param  dt_ms 控制周期，单位毫秒。
 * @note   先 GimbalAxis_Init(&axis, &config) 注册电机，任务每周期直接调用：
 *         Gm6020_TestAngleLoop_Run(&axis.motor, &axis.config.calibration,
 *                                  &axis, &test, HAL_GetTick(), 2U);
 *         test 置零，指定轴名称、目标度数和日志间隔；不需要测试 Init。
 *         只在任务上下文调用，禁止 CAN ISR 调用；motor 必须为 &axis.motor。
 *         PID、Ramp、保护参数改对应轴配置，确认后正式模式使用同一组参数。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void Gm6020_TestAngleLoop_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    GimbalAxis_HandleTypeDef *axis,
    Gm6020_TestAngleLoopTypeDef *test,
    uint32_t now_ms,
    uint32_t dt_ms);

#endif /* TEST_GM6020_ANGLE_LOOP_H */
