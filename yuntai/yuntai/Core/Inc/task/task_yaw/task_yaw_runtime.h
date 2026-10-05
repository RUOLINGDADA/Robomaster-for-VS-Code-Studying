/**
  ******************************************************************************
  * @file    task_yaw_runtime.h
  * @brief   Yaw 任务的配置适配；所有控制和保护均由 app/gimbal 共用。
  *
  * 通俗理解：这里只负责把 Yaw 的 CAN ID、标定和参数接到通用云台轴，
  * 不在任务里复制另一套 PID 或堵转算法。
  *
  * 通俗理解：这里负责把 Yaw 的 CAN ID、标定和参数接到通用云台轴，
  * 不在任务里复制另一套 PID 或堵转算法。
  ******************************************************************************
  */
#ifndef TASK_YAW_RUNTIME_H
#define TASK_YAW_RUNTIME_H /* 防止 Yaw 运行时重复包含（避免运行时结构重复定义）。 */
#include "app/gimbal/gimbal_axis.h"
#include "bsp/gm6020/test_gm6020_calibration.h"
#include "bsp/gm6020/test_gm6020_angle_loop.h"
typedef struct {
  GimbalAxis_HandleTypeDef axis; /* 本轴独立正式运行时，含 GM6020 句柄（Yaw 不与 Pitch 共用）。 */
  Gm6020_TestCalibrationTypeDef calibration_test; /* 本轴提示与标定日志状态（标定模式使用）。 */
  Gm6020_TestAngleLoopTypeDef angle_test; /* 本轴固定目标和限频日志状态（角度测试使用）。 */
  uint32_t last_log_ms; /* 正式模式日志时间，单位 HAL ms（只限制日志频率）。 */
} YawTask_RuntimeTypeDef;
/**
 * @brief  准备本轴配置和测试参数并注册 GM6020（只在任务入口做一次）。
 * @param  runtime 任务静态运行时对象（保存本轴全部状态）。
 * @retval true 成功；false ID 重复或配置指针无效（失败时轴保持零输出）。
 * @note   只在任务入口初始化一次，CAN 由 main/CubeMX 初始化（这里不启动外设）。
 */
bool YawTask_RuntimeInit(YawTask_RuntimeTypeDef *runtime);
/**
 * @brief  正式模式执行一个周期（读取命令后交给通用轴运行时）。
 * @param  runtime 本轴任务独占对象（不能与另一任务共享）。
 * @param  now_ms HAL_GetTick() 当前 ms（用于命令和反馈超时）。
 * @param  dt_ms 控制周期 ms（用于控制器换算）。
 * @note   Yaw 命令超时检查后交给正式共用运行时（过期命令自动保持当前位置）。
 * @retval None 运行结果保存在本轴对象；只在本轴任务调用。
 */
void YawTask_RuntimeRunCycle(YawTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms);
#endif /* TASK_YAW_RUNTIME_H（防止 Yaw 运行时接口被重复包含） */
