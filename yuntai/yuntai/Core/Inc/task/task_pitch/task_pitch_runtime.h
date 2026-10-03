/**
  ******************************************************************************
  * @file    task_pitch_runtime.h
  * @brief   Pitch 任务的配置适配；所有控制和保护均由 app/gimbal 共用。
  ******************************************************************************
  */
#ifndef TASK_PITCH_RUNTIME_H
#define TASK_PITCH_RUNTIME_H /* 防止 Pitch 运行时重复包含。 */
#include "app/gimbal/gimbal_axis.h"
#include "bsp/gm6020/test_gm6020_calibration.h"
#include "bsp/gm6020/test_gm6020_angle_loop.h"
typedef struct {
  GimbalAxis_HandleTypeDef axis; /* 本轴独立正式运行时，含 GM6020 句柄。 */
  Gm6020_TestCalibrationTypeDef calibration_test; /* 本轴提示与标定日志状态。 */
  Gm6020_TestAngleLoopTypeDef angle_test; /* 本轴固定目标和限频日志状态。 */
  uint32_t last_log_ms; /* 正式模式日志时间，单位 HAL ms。 */
} PitchTask_RuntimeTypeDef;
/**
 * @brief  准备本轴配置和测试参数并注册 GM6020。
 * @param  runtime 任务静态运行时对象。
 * @retval true 成功；false ID 重复或配置指针无效。
 * @note   只在任务入口初始化一次，CAN 由 main/CubeMX 初始化。
 */
bool PitchTask_RuntimeInit(PitchTask_RuntimeTypeDef *runtime);
/**
 * @brief  正式模式执行一个周期。
 * @param  runtime 本轴任务独占对象。
 * @param  now_ms HAL_GetTick() 当前 ms。
 * @param  dt_ms 控制周期 ms。
 * @note   Pitch 尚无真实输入协议，保持零输出。
 * @retval None 只修改本轴零输出命令；只在本轴任务调用。
 */
void PitchTask_RuntimeRunCycle(PitchTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms);
#endif /* TASK_PITCH_RUNTIME_H */
