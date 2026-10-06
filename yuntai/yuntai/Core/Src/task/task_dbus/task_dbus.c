/**
 * @file task_dbus.c
 * @brief DBUS 输入 FreeRTOS 入口与周期调度。
 *
 * PC11/USART3 的 18 字节 DBUS 协议。
 * CubeMX/.ioc 创建该任务：Normal3，栈 2048 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * Normal3 使输入任务先于 Normal2 电机任务运行；该顺序不替代各任务的超时检查。
 * 先由 main 完成 USART3/DMA 初始化，再启动接收；尚未同步或无合法帧时不发布有效输入。
 * 初始化失败每 100 ms 重试；接收故障的后续恢复由 Dbus_Process 在任务中处理。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "bsp/dbus/dbus.h"
#include "task/task_dbus/task_dbus.h"
#include "task/task_dbus/task_dbus_config.h"
#include "task/task_dbus/task_dbus_runtime.h"

/* 配置周期须至少为一个 Tick；换算为 0 会使输入任务忙循环，挤占电机控制。 */
_Static_assert(pdMS_TO_TICKS(DBUS_TASK_PERIOD_MS) > 0U,
               "DBUS task period must be at least one tick");

/**
 * @brief 启动接收，每 2 ms 调度 DBUS 解码与三路命令发布。
 * @param argument 未使用的 FreeRTOS 任务参数。
 * @retval None 任务永不返回；初始化失败时限频重试，云台与供弹均收不到有效命令。
 * @note 由 CubeMX 创建，优先级 Normal3，栈 2048 words（来源 yuntai.ioc）；ISR 不调用此入口。
 */
void task_dbus_entry(void *argument) {
  (void)argument;
  /* CubeMX 已配置 USART3 与 DMA，本任务只启动 BSP 接收。启动失败让出 CPU，等待下一次重试。
   * 不用 HAL_Delay；任务阻塞期间其它控制任务仍可检查命令过期。（收不到输入就不续期。） */
  while (!Dbus_Init()) {
    vTaskDelay(pdMS_TO_TICKS(DBUS_TASK_INIT_RETRY_MS));
  }
  TickType_t last_wake = xTaskGetTickCount(); /* 启动成功后建立 Tick 调度基准，与帧时间戳独立。 */
  for (;;) {
    /* 每次先处理接收与恢复，再发布完整命令；没有新帧时仍保留原接收时间。
     * 输入任务先运行不能保证电机任务总能读到本周期新帧，所以邮箱自身必须检查年龄。 */
    DbusTask_RuntimeRunCycle(HAL_GetTick());
    /* 固定绝对唤醒时刻，避免日志耗时累积到输入周期（HAL ms 只用于帧年龄，不拿 Tick 与它相减）。 */
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DBUS_TASK_PERIOD_MS));
  }
}
