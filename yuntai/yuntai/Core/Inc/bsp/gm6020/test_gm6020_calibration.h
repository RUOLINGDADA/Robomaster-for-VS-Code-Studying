/**
  ******************************************************************************
  * @file    test_gm6020_calibration.h
  * @brief   可复用的 GM6020 手动机械角度标定测试接口。
  *
  * Yaw 和 Pitch 均使用同一套只读标定逻辑。测试永远关闭输出，只通过
  * Gm6020_GetSnapshot() 读取一致反馈；用户把日志中的连续角度写入各轴配置。
  ******************************************************************************
  */

#ifndef TEST_GM6020_CALIBRATION_H
#define TEST_GM6020_CALIBRATION_H /* 防止 GM6020 标定接口重复包含。 */

#include "bsp/gm6020/gm6020.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  const char *axis_name; /* 日志轴名称，例如“水平轴”或“发射云台”。 */
  const char *first_limit_name; /* 第一个物理边界，例如“左限位”或“下限位”。 */
  const char *second_limit_name; /* 第二个物理边界，例如“右限位”或“上限位”。 */
  uint32_t log_period_ms; /* 数据日志间隔 ms；0 使用默认 200 ms。 */
  uint32_t prompt_period_ms; /* 操作提示间隔 ms；0 使用默认 5000 ms。 */
  uint32_t last_log_ms; /* 角度日志时间戳，单位 HAL 毫秒。 */
  uint32_t last_prompt_ms; /* 操作提示时间戳，单位 HAL 毫秒。 */
  bool log_started; /* true 表示至少一条数据日志成功提交。 */
  bool prompt_printed; /* true 表示已经打印过首轮操作提示。 */
} Gm6020_TestCalibrationTypeDef;

/**
 * @brief  执行一次非阻塞的手动角度标定。
 * @param  motor 已初始化并注册接收的 GM6020 句柄；函数不会修改标定值。
 * @note   调用示例：任务初始化电机后，每周期直接调用本函数；
 *         test 置零，只指定 axis_name 和两侧名称即可，不需要测试 Init。
 * @param  calibration 该轴标定值，仅用于日志状态提示。
 * @param  test 该轴独立的测试状态，不能在 Yaw/Pitch 间共享。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒。
 * @note   只能在任务上下文调用；每次调用发送零电流，禁止在 CAN ISR 调用。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void Gm6020_TestCalibration_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    Gm6020_TestCalibrationTypeDef *test,
    uint32_t now_ms);

#endif /* TEST_GM6020_CALIBRATION_H */
