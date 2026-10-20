/**
 * @file task_ins.c
 * @brief 独立 INS 任务强入口；任务属性由用户在 CubeMX 创建。
 * DRDY/DMA 事件阻塞调度代替固定延时轮询，不创建额外任务。
 */
#include "task/task_ins/task_ins.h"
#include "task/task_ins/task_ins_runtime.h"
#include "task/task_ins/task_ins_config.h"
#include "FreeRTOS.h"
#include "task.h"

void task_ins_entry(void *argument) {
  (void)argument;
  if (!InsTask_RuntimeInit()) {
    for (;;) { vTaskDelay(pdMS_TO_TICKS(INS_RETRY_MS)); }
  }
  for (;;) { InsTask_RuntimeRunCycle(); }
}
