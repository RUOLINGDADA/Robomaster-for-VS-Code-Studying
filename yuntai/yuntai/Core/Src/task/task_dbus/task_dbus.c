/**
 * @file task_dbus.c
 * @brief DBUS FreeRTOS 入口（调度接收任务，不把控制器搬进输入任务）。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "bsp/dbus/dbus.h"
#include "task/task_dbus/task_dbus.h"
#include "task/task_dbus/task_dbus_config.h"
#include "task/task_dbus/task_dbus_runtime.h"

_Static_assert(pdMS_TO_TICKS(DBUS_TASK_PERIOD_MS) > 0U,
               "DBUS task period must be at least one tick");

void task_dbus_entry(void *argument) {
  (void)argument;
  while (!Dbus_Init()) {
    vTaskDelay(pdMS_TO_TICKS(DBUS_TASK_INIT_RETRY_MS));
  }
  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    DbusTask_RuntimeRunCycle(HAL_GetTick());
    /* 固定绝对唤醒时刻，避免日志耗时累积到输入周期（HAL ms 只用于帧年龄，不拿 Tick 与它相减）。 */
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DBUS_TASK_PERIOD_MS));
  }
}
