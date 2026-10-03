/**
  ******************************************************************************
  * @file    task_feed_motor.c
  * @brief   M2006 供弹任务的 FreeRTOS 入口和固定周期调度。
  *
  * 本文件只负责创建运行时对象、获取 HAL 毫秒时间和 FreeRTOS Tick，
  * 然后按固定 2 ms 周期调用驱动同级硬件调参函数或正式运行函数。测试阶段位于
  * bsp/c610_m2006，反馈安全保护和正式运行时仍位于 task_feed_motor 目录，
  * 便于以后替换输入而不修改 CubeMX 任务创建代码。
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "task/task_feed_motor/task_feed_motor.h"
#include "task/task_feed_motor/task_feed_motor_runtime.h"
#include "bsp/c610_m2006/test_c610_m2006_self_cycle.h"
#include "usart.h"

/* 任务唤醒周期；单位毫秒，必须与 runtime 的 dt_ms 参数保持一致。 */
#define FEED_MOTOR_TASK_PERIOD_MS 2U /* 供弹控制任务唤醒周期，单位毫秒。 */

/**
 * @brief  执行唯一 M2006 供弹电机的周期任务。
 * @param  argument CubeMX/FreeRTOS 任务参数，当前未使用。
 * @retval None 任务入口不会返回。
 */
void task_feed_motor_entry(void *argument) {
  (void)argument;
  static FeedMotor_RuntimeTypeDef runtime;

  if (!FeedMotor_RuntimeInit(&runtime)) {
    usart_printf("[供弹] 运行时初始化失败，保持零输出\r\n");
    for (;;) {
      vTaskDelay(pdMS_TO_TICKS(1000U));
    }
  }

  TickType_t last_wake_tick = xTaskGetTickCount();
  for (;;) {
    /* HAL Tick 用于反馈超时，FreeRTOS Tick 只用于任务调度和阶段计时。 */
#if C610_M2006_HARDWARE_TEST_ENABLE
    /* 硬件调参模式：唯一供弹电机执行非阻塞上弹/停止/下弹自循环。 */
    C610_M2006_TestSelfCycle_Run(&runtime.motor, HAL_GetTick());
#else
    /*
     * 正式模式：执行供弹命令、阶段控制和保护逻辑；测试自循环不进入此路径。
     */
    FeedMotor_RuntimeRunCycle(&runtime, HAL_GetTick(),
                              xTaskGetTickCount());
#endif
    vTaskDelayUntil(&last_wake_tick,
                    pdMS_TO_TICKS(FEED_MOTOR_TASK_PERIOD_MS));
  }
}
