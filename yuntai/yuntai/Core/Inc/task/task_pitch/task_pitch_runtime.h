/**
  ******************************************************************************
  * @file    task_pitch_runtime.h
  * @brief   Pitch 任务的配置适配；所有控制和保护均由 app/gimbal 共用。
  *
  * 通俗理解：Pitch 只负责提供本轴 CAN ID、标定和参数，实际控制流程与 Yaw 相同。
  *
  * 通俗理解：Pitch 只负责提供本轴 CAN ID、标定和参数，实际控制流程与 Yaw 相同。
  ******************************************************************************
  */
#ifndef TASK_PITCH_RUNTIME_H
#define TASK_PITCH_RUNTIME_H /* 防止 Pitch 运行时重复包含（避免运行时结构重复定义）。 */
#include "app/gimbal/gimbal_axis.h"
#include "bsp/gm6020/test_gm6020_calibration.h"
#include "bsp/gm6020/test_gm6020_angle_loop.h"
typedef struct {
  GimbalAxis_HandleTypeDef axis; /* 本轴独立正式运行时，含 GM6020 句柄（Pitch 不与 Yaw 共用）。 */
  Gm6020_TestCalibrationTypeDef calibration_test; /* 本轴提示与标定日志状态（标定模式使用）。 */
  Gm6020_TestAngleLoopTypeDef angle_test; /* 本轴固定目标和限频日志状态（角度测试使用）。 */
  uint32_t last_log_ms; /* 正式模式日志时间，单位 HAL ms（只限制日志频率）。 */
} PitchTask_RuntimeTypeDef;
/**
 * @brief  准备本轴配置和测试参数并注册 GM6020（只在任务入口做一次）。
 * @param  runtime 任务静态运行时对象（保存本轴全部状态）。
 * @retval true 成功；false ID 重复或配置指针无效（失败时轴保持零输出）。
 * @note   只在任务入口初始化一次，CAN 由 main/CubeMX 初始化（这里不启动外设）。
 */
bool PitchTask_RuntimeInit(PitchTask_RuntimeTypeDef *runtime);
/**
 * @brief  正式模式执行一个周期（读取输入后交给通用轴运行时）。
 * @param  runtime 本轴任务独占对象（不能与另一任务共享）。
 * @param  now_ms HAL_GetTick() 当前 ms（用于反馈超时和日志）。
 * @param  dt_ms 控制周期 ms（用于控制器换算）。
 * @note   读取 DBUS 发布的独立命令；未标定或过期走零输出，有效回中保持目标（日志使用同周期快照）。
 * @retval None 更新本轴控制、保护和 CAN 输出；只在本轴任务调用。
 */
void PitchTask_RuntimeRunCycle(PitchTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms);
#endif /* TASK_PITCH_RUNTIME_H（防止 Pitch 运行时接口被重复包含） */
