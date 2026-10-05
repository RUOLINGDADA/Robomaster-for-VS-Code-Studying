/**
  ******************************************************************************
  * @file    test_gm6020_calibration.h
  * @brief   可复用的 GM6020 手动机械角度标定测试接口（只读反馈，不驱动电机）。
  *
  * Yaw 和 Pitch 均使用同一套只读标定逻辑（两个轴只替换名称和自己的状态）。
  * 测试永远关闭输出，只通过 Gm6020_GetSnapshot() 读取一致反馈；用户把日志中的
  * 连续角度写入各轴配置（手动转到两个边界和中心，记录日志中的 angle_total_raw）。
  ******************************************************************************
  */

#ifndef TEST_GM6020_CALIBRATION_H
#define TEST_GM6020_CALIBRATION_H /* 防止 GM6020 标定接口重复包含（避免结构体和函数声明重复）。 */

#include "bsp/gm6020/gm6020.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  const char *axis_name; /* 日志轴名称，例如“水平轴”或“发射云台”（只读静态字符串）。 */
  const char *first_limit_name; /* 第一个物理边界，例如“左限位”或“下限位”（提示操作者先记录哪一侧）。 */
  const char *second_limit_name; /* 第二个物理边界，例如“右限位”或“上限位”（提示操作者再记录哪一侧）。 */
  uint32_t log_period_ms; /* 数据日志最短间隔，单位 HAL ms；0 使用默认 200 ms（避免串口刷屏）。 */
  uint32_t prompt_period_ms; /* 操作提示最短间隔，单位 HAL ms；0 使用默认 5000 ms（定期提醒记录）。 */
  uint32_t last_log_ms; /* 上次角度日志时间戳，单位 HAL ms（只用于限频）。 */
  uint32_t last_prompt_ms; /* 上次操作提示时间戳，单位 HAL ms（只用于限频）。 */
  bool log_started; /* true 表示至少一条数据日志成功提交（DMA 忙时保持 false，下一周期重试）。 */
  bool prompt_printed; /* true 表示已经打印过首轮操作提示（避免刚启动就重复打印）。 */
} Gm6020_TestCalibrationTypeDef;

/**
 * @brief  执行一次非阻塞的手动角度标定（每次只读一份反馈并尝试发零电流）。
 * @param  motor 已初始化并注册接收的 GM6020 句柄；函数不会修改标定值（只访问驱动反馈）。
 * @param  calibration 该轴标定值，仅用于日志状态提示（不会写回配置）。
 * @param  test 该轴独立的测试状态，不能在 Yaw/Pitch 间共享（各轴分别记录限频时间）。
 * @param  now_ms HAL_GetTick() 当前时间，单位毫秒（必须与反馈时间戳同源）。
 * @note   调用者应在任务初始化电机后每周期调用；test 可先清零，不需要额外 Init。
 *         只能在任务上下文调用；每次调用发送零电流，禁止在 CAN ISR 调用（ISR 只收帧）。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void Gm6020_TestCalibration_Run(
    Gm6020_HandleTypeDef *motor,
    const Gm6020_CalibrationTypeDef *calibration,
    Gm6020_TestCalibrationTypeDef *test,
    uint32_t now_ms);

#endif /* TEST_GM6020_CALIBRATION_H */
