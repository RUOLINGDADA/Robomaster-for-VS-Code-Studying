/**
  ******************************************************************************
  * @file    task_pitch.c
  * @brief   Pitch FreeRTOS 入口：初始化、2 ms 周期调度和一个模式调用。
  *
  * 参数统一放 task_pitch_config.h；测试与正式模式共用同一个轴实例。
  * 通俗理解：任务只负责每 2 ms 按当前模式调用一次，控制细节都在运行时模块。
  * 函数只在任务中运行，CAN ISR 只接收反馈，不执行测试或日志。
  ******************************************************************************
  */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "usart.h"
#include "task/task_pitch/task_pitch.h"
#include "task/task_pitch/task_pitch_config.h"
#include "task/task_pitch/task_pitch_runtime.h"

/* 周期至少换成一个 Tick 才能让出 CPU；小于一个 Tick 会使调度与控制 dt 脱节（配置错误在编译时拒绝）。 */
_Static_assert(PITCH_TASK_PERIOD_MS > 0U && pdMS_TO_TICKS(PITCH_TASK_PERIOD_MS) > 0U,
               "Pitch task period must be at least one FreeRTOS tick");

void task_pitch_entry(void *argument) {
  (void)argument;
  static PitchTask_RuntimeTypeDef runtime;
  if (!PitchTask_RuntimeInit(&runtime)) {
    for (;;) {
      usart_printf("[发射云台] 初始化失败，保持零输出\r\n");
      vTaskDelay(pdMS_TO_TICKS(1000U));
    }
  }
  TickType_t last_wake_tick = xTaskGetTickCount();
  for (;;) {
    const uint32_t now_ms = HAL_GetTick(); /* CAN 时间戳与诊断都使用 HAL ms（反馈超时和日志都依赖它）。 */
#if PITCH_HARDWARE_TEST_MODE == PITCH_HARDWARE_TEST_MODE_CALIBRATION
    /* 上板标定：只读打印，始终零电流（人手移动记录中心和边界）。 */
    Gm6020_TestCalibration_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.calibration_test, now_ms);
#elif PITCH_HARDWARE_TEST_MODE == PITCH_HARDWARE_TEST_MODE_ANGLE_LOOP
    /* 上板调参：固定目标；正式角度环、速度 PI、Ramp 和保护全部复用（测试和正式行为一致）。 */
    Gm6020_TestAngleLoop_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.axis,
        &runtime.angle_test, now_ms, PITCH_TASK_PERIOD_MS);
#else
    /* 正式模式：右摇杆上下输入进入同一轴运行时；未标定、掉线零输出（不照搬 Yaw 的实测范围）。 */
    PitchTask_RuntimeRunCycle(&runtime, now_ms, PITCH_TASK_PERIOD_MS);
#endif
    /* 以绝对 Tick 保持配置周期（执行完再延时会把日志耗时累积成周期漂移）。 */
    vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(PITCH_TASK_PERIOD_MS));
  }
}
