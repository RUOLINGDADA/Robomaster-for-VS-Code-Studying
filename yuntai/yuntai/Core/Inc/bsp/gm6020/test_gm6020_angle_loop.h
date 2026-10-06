/**
 * @file test_gm6020_angle_loop.h
 * @brief GM6020 的固定目标角度硬件测试。
 *
 * 先初始化 GimbalAxis，再由所属轴任务调用；目标为相对中心的 deg。
 * 检查标定和目标范围，随后复用正式闭环、Ramp 与边界过滤。无效目标发零电流。
 * 控制和任务日志共用一次反馈快照。本模块不复制控制算法，操作者须能立即断电。
 */

#ifndef TEST_GM6020_ANGLE_LOOP_H
#define TEST_GM6020_ANGLE_LOOP_H /* 防止 GM6020 角度测试接口重复包含。避免结构体和声明重复。 */

#include "app/gimbal/gimbal_axis.h"

#include "app/log/log.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  Log_CategoryTypeDef log_category; /* 所属轴的 YAW_TEST/PITCH_TEST 分类；零值不输出。 */
  const char *axis_name; /* 日志轴名称，不能使用共享可变字符串。每轴应有自己的静态名称。 */
  float target_angle_deg; /* 相对本轴中位的固定目标，单位度。可为负数和小数。 */
  uint32_t log_period_ms; /* 诊断日志最短间隔，单位 ms。0 时由实现使用默认值。 */
  uint32_t last_log_ms; /* 本实例上一次日志时间，单位 HAL 毫秒。只用于限频。 */
  bool log_started; /* true 表示该实例至少成功输出一条日志。DMA 忙时保持 false。 */
  bool target_warning_logged; /* 防止无效配置提示刷屏。同一配置只提示一次。 */
} Gm6020_TestAngleLoopTypeDef;

/**
 * @brief  执行一次固定目标角度环硬件调参。沿用正式控制、Ramp 和软件边界过滤。
 * @param  motor 该轴 GM6020 句柄，用于读取本周期一致快照。必须等于 &axis->motor。
 * @param  calibration 该轴中心与边界配置。函数检查它是否仍与 axis 配置一致。
 * @param  axis 正式通用角度控制实例，不能与其它轴共享。测试直接复用这份历史状态。
 * @param  test 该轴独立测试状态和目标角度参数。Yaw/Pitch 不能共用。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。用于反馈年龄和日志限频。
 * @param  dt_ms 控制周期，单位毫秒。必须大于零，传给角度/速度环。
 * @note   先 GimbalAxis_Init(&axis, &config) 注册电机，任务每周期直接调用：
 * Gm6020_TestAngleLoop_Run(&axis.motor, &axis.config.calibration,
 * &axis, &test, HAL_GetTick(), 2U);
 * test 置零，指定本轴日志分类、轴名称、目标度数和日志间隔。不需要测试 Init。
 * 只在任务上下文调用，禁止 CAN ISR 调用。motor 必须为 &axis.motor。避免读错轴。
 * PID、Ramp 和边界参数改对应轴配置，确认后正式模式使用同一组参数。测试不复制算法。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void Gm6020_TestAngleLoop_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    GimbalAxis_HandleTypeDef *axis,
    Gm6020_TestAngleLoopTypeDef *test,
    uint32_t now_ms,
    uint32_t dt_ms);

#endif /* TEST_GM6020_ANGLE_LOOP_H。 */
