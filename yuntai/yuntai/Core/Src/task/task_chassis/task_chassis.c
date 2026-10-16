/**
 * @file task_chassis.c
 * @brief 底盘唯一任务的初始化与固定节拍。
 *
 * main 先启动 CAN1/CAN2；CubeMX 手动创建本任务，Normal2，1024 words。
 * Ctrl 模式、四轮 PID 和输出全部由运行时组合，不创建模式管理或小陀螺任务。
 */
#include "task/task_chassis/task_chassis.h"
#include "task/task_chassis/task_chassis_config.h"
#include "task/task_chassis/task_chassis_runtime.h"
#include "FreeRTOS.h"
#include "task.h"

static ChassisTask_RuntimeTypeDef g_runtime; /* 生命周期贯穿系统运行；CAN 注册表长期引用其中四轮句柄。 */

_Static_assert(pdMS_TO_TICKS(CHASSIS_TASK_PERIOD_MS) > 0U, "chassis period must be at least one Tick");
_Static_assert(pdMS_TO_TICKS(CHASSIS_INIT_RETRY_MS) > 0U, "chassis retry must be at least one Tick");

/* 入口只负责初始化与调度；实际控制间隔异常由运行时清零，不连续补发旧周期。 */
void task_chassis_entry(void *argument) {
  (void)argument;
  while (!ChassisTask_RuntimeInit(&g_runtime)) { vTaskDelay(pdMS_TO_TICKS(CHASSIS_INIT_RETRY_MS)); }
  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    ChassisTask_RuntimeRunCycle(&g_runtime, HAL_GetTick());
    if (xTaskGetTickCount() - last_wake > pdMS_TO_TICKS(CHASSIS_TASK_PERIOD_MS)) {
      last_wake = xTaskGetTickCount(); /* 超期重设绝对节拍，避免补跑多个过期控制周期。 */
    }
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CHASSIS_TASK_PERIOD_MS));
  }
}
