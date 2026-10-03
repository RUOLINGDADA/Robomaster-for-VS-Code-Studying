/**
  ******************************************************************************
  * @file    task_yaw.c
  * @brief   Yaw FreeRTOS 入口：初始化、2 ms 周期调度和一个模式调用。
  *
  * 参数统一放 task_yaw_config.h；测试与正式模式共用同一个轴实例。
  * 函数只在任务中运行，CAN ISR 只接收反馈，不执行测试或日志。
  ******************************************************************************
  */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "usart.h"
#include "task/task_yaw/task_yaw.h"
#include "task/task_yaw/task_yaw_config.h"
#include "task/task_yaw/task_yaw_runtime.h"

#define YAW_TASK_PERIOD_MS 2U /* 本轴任务绝对唤醒周期，单位 ms。 */

void task_yaw_entry(void *argument) {
  (void)argument;
  static YawTask_RuntimeTypeDef runtime;
  if (!YawTask_RuntimeInit(&runtime)) {
    for (;;) {
      usart_printf("[水平轴] 初始化失败，保持零输出\r\n");
      vTaskDelay(pdMS_TO_TICKS(1000U));
    }
  }
  TickType_t last_wake_tick = xTaskGetTickCount();
  for (;;) {
    const uint32_t now_ms = HAL_GetTick(); /* CAN 时间戳与诊断都使用 HAL ms。 */
#if YAW_HARDWARE_TEST_MODE == YAW_HARDWARE_TEST_MODE_CALIBRATION
    /* 上板标定：只读打印，始终零电流。 */
    Gm6020_TestCalibration_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.calibration_test, now_ms);
#elif YAW_HARDWARE_TEST_MODE == YAW_HARDWARE_TEST_MODE_ANGLE_LOOP
    /* 上板调参：固定目标；正式角度环、速度 PI、Ramp 和保护全部复用。 */
    Gm6020_TestAngleLoop_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.axis,
        &runtime.angle_test, now_ms, YAW_TASK_PERIOD_MS);
#else
    /* 正式模式：真实 Yaw 命令经过同一个运行时控制链。 */
    YawTask_RuntimeRunCycle(&runtime, now_ms, YAW_TASK_PERIOD_MS);
#endif
    vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(YAW_TASK_PERIOD_MS));
  }
}
