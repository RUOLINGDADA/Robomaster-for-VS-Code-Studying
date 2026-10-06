/**
 * @file task_yaw_runtime.h
 * @brief Yaw配置、命令与共用轴运行时适配。
 *
 * CAN1 反馈 0x205 的独立 GM6020。
 * 仅所属轴任务调用；Yaw/Pitch 不共享句柄、控制历史或测试状态。
 * HAL ms 用于命令、反馈和日志。dt_ms 用于控制周期，不传 FreeRTOS Tick。
 * 命令失效按零速度保持。未标定、参数无效或 CAN 掉线时清零电流。
 * 控制与日志共用 GimbalAxis 的周期快照。本模块不复制 PID 或 CAN 协议。
 */
/* 调用顺序：main 外设初始化→所属任务 RuntimeInit→成功后周期 RunCycle。
 * 对象须保持固定地址；不得由其它任务再次初始化或复用。日志只能读本周期副本。 */
#ifndef TASK_YAW_RUNTIME_H
#define TASK_YAW_RUNTIME_H /* 防止 Yaw 运行时重复包含（避免运行时结构重复定义）。 */
#include "app/gimbal/gimbal_axis.h"
#include "bsp/gm6020/test_gm6020_calibration.h"
#include "bsp/gm6020/test_gm6020_angle_loop.h"
/* 整个对象由所属轴任务独占；Init 仅一次，之后禁止 memcpy 到新地址或清空内部 motor。
 * CAN ISR 只经驱动更新 axis.motor，任务和日志使用 axis.cycle 的一致副本。 */
typedef struct {
  GimbalAxis_HandleTypeDef axis; /* 本轴独立正式运行时，含 GM6020 句柄（Yaw 不与 Pitch 共用）。 */
  Gm6020_TestCalibrationTypeDef calibration_test; /* 任务独占标定提示/日志状态；Init 清零，标定模式仍发送零电流，不自动写回配置。 */
  Gm6020_TestAngleLoopTypeDef angle_test; /* 任务独占固定目标 deg 与日志历史；Init 清零后复制参数，只在角度测试模式推进。 */
  uint32_t last_log_ms; /* 最近成功日志时间，HAL ms；Init=0，仅 UART 接受文本后更新，不影响控制时钟。 */
} YawTask_RuntimeTypeDef;
/**
 * @brief  准备本轴配置和测试参数并注册 GM6020（只在任务入口做一次）。
 * @param  runtime 任务静态运行时对象（保存本轴全部状态）。
 * @retval true 成功；false 指针无效、CAN/ID 非法、ID 重复或注册表已满；入口不进入控制，不保证零帧已送达。
 * @note   只在任务入口初始化一次，CAN 由 main/CubeMX 初始化（这里不启动外设）。
 */
bool YawTask_RuntimeInit(YawTask_RuntimeTypeDef *runtime);
/**
 * @brief  正式模式执行一个周期（读取命令后交给通用轴运行时）。
 * @param  runtime 本轴任务独占对象（不能与另一任务共享）。
 * @param  now_ms HAL_GetTick() 当前 ms（用于命令和反馈超时）。
 * @param  dt_ms 控制周期 ms（用于控制器换算）。
 * @note   Yaw 命令超时检查后交给正式共用运行时（过期命令冻结最后目标位置）。
 * @retval None 运行结果保存在本轴对象；只在本轴任务调用。
 */
void YawTask_RuntimeRunCycle(YawTask_RuntimeTypeDef *runtime,
                             uint32_t now_ms, uint32_t dt_ms);
#endif /* TASK_YAW_RUNTIME_H（防止 Yaw 运行时接口被重复包含） */
