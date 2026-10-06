/**
 * @file task_yaw.c
 * @brief Yaw FreeRTOS 入口与周期调度。
 *
 * CAN1 反馈 0x205 的独立 GM6020。
 * CubeMX/.ioc 创建该任务：Normal2，栈 2048 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * Normal2 与另一轴及供弹同级；输入由更高优先级 DBUS 任务发布，反馈由 CAN ISR 更新。
 * main 先配置并启动 CAN；任务初始化只注册独立轴，首帧到达后才建立控制历史。
 * 注册失败后每 1000 ms 尝试输出诊断并等待，不重试注册、不进入控制循环；须修正配置后复位。
 * 正式、只读标定、固定目标测试在编译时选一条路径，不能在同周期分别写电流。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "app/log/log.h"
#include "task/task_yaw/task_yaw.h"
#include "task/task_yaw/task_yaw_config.h"
#include "task/task_yaw/task_yaw_runtime.h"

/* 防止配置周期换成 Tick 后为零；否则任务忙循环，PID 的 dt 也和调度不一致（至少一个 Tick 才能按时让出 CPU）。 */
_Static_assert(YAW_TASK_PERIOD_MS > 0U && pdMS_TO_TICKS(YAW_TASK_PERIOD_MS) > 0U,
               "Yaw task period must be at least one FreeRTOS tick");

/**
 * @brief  执行 Yaw 水平轴控制任务（初始化后按固定周期选择测试或正式模式）。
 * @param  argument CubeMX/FreeRTOS 任务参数，当前未使用（保持入口签名不变）。
 * @retval None 任务入口不会返回（结束条件由系统复位或调度器停止决定）。
 * @note   仅由 FreeRTOS 创建和调度，不能在 CAN ISR 或其它任务直接调用（避免并发写控制状态）。
 */
void task_yaw_entry(void *argument) {
  (void)argument;
  static YawTask_RuntimeTypeDef runtime; /* 静态零初始化，地址在任务整个生命期有效；CAN 注册表保存内部句柄地址。 */
  /* 先完成配置复制和注册，再建立周期基准。失败路径只等待和诊断，不能调用未注册轴控制。
   * 日志文字描述软件禁用状态，不是一次成功 CAN 停机确认。（初始化未完成就不进入闭环。） */
  if (!YawTask_RuntimeInit(&runtime)) {
    for (;;) {
      (void)LOG_TRY_PRINTF(LOG_CATEGORY_YAW, "[水平轴] 初始化失败，保持零输出\r\n");
      vTaskDelay(pdMS_TO_TICKS(1000U));
    }
  }
  TickType_t last_wake_tick = xTaskGetTickCount(); /* 初始化成功后建立绝对唤醒基准，初始化耗时不计入首周期。 */
  for (;;) {
    const uint32_t now_ms = HAL_GetTick(); /* CAN 时间戳与诊断都使用 HAL ms（避免与 FreeRTOS Tick 混算）。 */
    /* 下列 #if 在编译时删除其它模式；不会在运行中切换，也不会同时执行多种电流输出。
     * 固定目标测试不受 DBUS 按钮控制；重新烧录模式前须确认标定和急停。 */
#if YAW_HARDWARE_TEST_MODE == YAW_HARDWARE_TEST_MODE_CALIBRATION
    /* 上板标定：只读打印，始终零电流（人手移动记录中心和边界）。 */
    Gm6020_TestCalibration_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.calibration_test, now_ms);
#elif YAW_HARDWARE_TEST_MODE == YAW_HARDWARE_TEST_MODE_ANGLE_LOOP
    /* 上板调参：固定目标；正式位置/速度 PID、Ramp 和安全门全部复用（测试和正式行为一致）。 */
    Gm6020_TestAngleLoop_Run(&runtime.axis.motor,
        &runtime.axis.config.calibration, &runtime.axis,
        &runtime.angle_test, now_ms, YAW_TASK_PERIOD_MS);
#else
    /* 正式模式：真实 Yaw 命令经过同一个运行时控制链（任务不复制控制算法）。 */
    YawTask_RuntimeRunCycle(&runtime, now_ms, YAW_TASK_PERIOD_MS);
#endif
    /* 使用配置周期的绝对唤醒点而非“执行完再延时”（避免日志耗时造成周期逐步漂移）。 */
    /* 这里只阻塞当前任务，CAN/DMA ISR 和其它任务继续工作。迟到周期不会被自动补偿。
     * 云台控制传入配置的 2 ms dt，未测量执行延迟；需保证本周期耗时小于周期。 */
    vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(YAW_TASK_PERIOD_MS));
  }
}
