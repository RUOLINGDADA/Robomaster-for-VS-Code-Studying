/**
 * @file task_feed_motor.c
 * @brief 供弹 FreeRTOS 入口与周期调度。
 *
 * CAN1 ID1 M2006 与 PE9/PE11 双 C615。
 * CubeMX/.ioc 创建该任务：Normal2，栈 1024 words。配置周期为 2 ms。
 * 入口只初始化并周期调度，不创建额外 RTOS 对象；任务永不返回。
 * FreeRTOS Tick 用于调度；HAL ms 用于命令和反馈年龄，不能混用。
 * 正式发射与显式硬件测试互斥。部分初始化失败保留停止输出，并限频重试。
 * Normal2 与两个云台任务同级；100 ms 命令/反馈安全门由本任务自行检查。
 * main 先完成 CAN1/TIM1；本任务注册 M2006、写停止值并启动双 PWM，不重复创建任务。
 * 初始化重试保留已注册句柄；HAL 失败不证明硬件已停，软件只能提交停止命令。
 */

#include "FreeRTOS.h"
#include "task.h"

#include "main.h"
#include "task/task_feed_motor/task_feed_motor.h"
#include "task/task_feed_motor/task_feed_motor_runtime.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include "bsp/c610_m2006/test_c610_m2006_self_cycle.h"
#include "bsp/snail_2305/test_snail_2305.h"
#include "bsp/snail_2305/test_snail_2305_calibration.h"
#include "tim.h"
#include "app/log/log.h"

#if FEED_MOTOR_TEST_ENABLE && (SNAIL_2305_TEST_ENABLE || \
                               FEED_MOTOR_SNAIL_MODE != FEED_MOTOR_SNAIL_MODE_FORMAL)
#error "C610 and Snail 2305 tests cannot run together"
#endif
#if SNAIL_2305_TEST_ENABLE && \
    FEED_MOTOR_SNAIL_MODE != FEED_MOTOR_SNAIL_MODE_FORMAL
#error "SNAIL_2305_TEST_ENABLE requires FEED_MOTOR_SNAIL_MODE=0"
#endif

_Static_assert(FEED_MOTOR_TEST_UP_CURRENT_RAW >= C610_M2006_CURRENT_RAW_MIN &&
               FEED_MOTOR_TEST_UP_CURRENT_RAW <= C610_M2006_CURRENT_RAW_MAX &&
               FEED_MOTOR_TEST_DOWN_CURRENT_RAW >= C610_M2006_CURRENT_RAW_MIN &&
               FEED_MOTOR_TEST_DOWN_CURRENT_RAW <= C610_M2006_CURRENT_RAW_MAX,
               "test currents exceed C610 protocol range");

/**
 * @brief  唯一 M2006 供弹电机的 FreeRTOS 任务入口（按固定周期推进控制）。
 * @param  argument CubeMX 传入的任务参数，当前未使用。
 * @retval None 任务初始化成功后永不返回；部分初始化失败时保留停止值，并每 100 ms 重试。
 * @note   只能由任务上下文执行。CAN 接收在 ISR 中完成，串口诊断在任务中限频执行。
 */
void task_feed_motor_entry(void *argument) {
  (void)argument;
#if FEED_MOTOR_SNAIL_MODE != FEED_MOTOR_SNAIL_MODE_FORMAL
  static Snail2305_CalibrationStateTypeDef snail_calibration;
  while (!Snail2305_Calibration_Init(&snail_calibration, &htim1)) {
    (void)LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
        "[C615校准] TIM1 或 PWM 初始化失败，保持最小脉宽并重试\r\n");
    vTaskDelay(pdMS_TO_TICKS(FEED_MOTOR_INIT_RETRY_MS));
  }
  TickType_t last_wake_tick = xTaskGetTickCount();
  for (;;) {
    const TickType_t now_tick = xTaskGetTickCount();
    const TickType_t elapsed_tick = now_tick - last_wake_tick;
    uint32_t dt_ms = (uint32_t)((uint64_t)elapsed_tick * 1000U / configTICK_RATE_HZ);
    if (dt_ms == 0U) {
      dt_ms = FEED_MOTOR_TASK_PERIOD_MS;
    }
    Snail2305_Calibration_RunCycle(&snail_calibration, HAL_GetTick(), dt_ms);
    vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(FEED_MOTOR_TASK_PERIOD_MS));
  }
#elif SNAIL_2305_TEST_ENABLE
  static Snail2305_TestStateTypeDef snail_test;
  while (!Snail2305_Test_Init(&snail_test, &htim1)) {
    (void)LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR_TEST,
        "[C615测试] TIM1 或 PWM 初始化失败，保持停止脉宽并重试\r\n");
    vTaskDelay(pdMS_TO_TICKS(FEED_MOTOR_INIT_RETRY_MS));
  }
  TickType_t last_wake_tick = xTaskGetTickCount();
  for (;;) {
    const TickType_t now_tick = xTaskGetTickCount();
    const TickType_t elapsed_tick = now_tick - last_wake_tick;
    uint32_t dt_ms = (uint32_t)((uint64_t)elapsed_tick * 1000U / configTICK_RATE_HZ);
    if (dt_ms == 0U) {
      dt_ms = FEED_MOTOR_TASK_PERIOD_MS;
    }
    Snail2305_Test_RunCycle(&snail_test, HAL_GetTick(), dt_ms);
    vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(FEED_MOTOR_TASK_PERIOD_MS));
  }
#else
  static FeedMotor_RuntimeTypeDef runtime; /* 唯一供弹任务独占，零初始化；PWM 与 CAN 句柄不可复制到其它实例。 */
#if FEED_MOTOR_TEST_ENABLE
  static const C610_M2006_TestConfigTypeDef test_config = {
      .enabled = true,
      .up_time_ms = FEED_MOTOR_TEST_UP_TIME_MS,
      .stop_time_ms = FEED_MOTOR_TEST_STOP_TIME_MS,
      .down_time_ms = FEED_MOTOR_TEST_DOWN_TIME_MS,
      .up_current_raw = FEED_MOTOR_TEST_UP_CURRENT_RAW,
      .down_current_raw = FEED_MOTOR_TEST_DOWN_CURRENT_RAW,
      .log_period_ms = FEED_MOTOR_TEST_LOG_PERIOD_MS,
  };
#endif

  /* 三个驱动全部初始化成功才允许进入周期控制。部分成功时运行时仍提交停止值。
   * 重试必须复用同一对象，不能重置已加入 CAN 注册表的句柄。（成功一半也不能发射。） */
  while (!FeedMotor_RuntimeInit(&runtime)) {
    (void)LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 初始化失败，停止输出并重试\r\n");
    vTaskDelay(pdMS_TO_TICKS(FEED_MOTOR_INIT_RETRY_MS));
  }

  TickType_t last_wake_tick = xTaskGetTickCount(); /* 初始化和重试结束后才开始周期调度，避免追赶启动耗时。 */
  for (;;) {
    /* HAL Tick 用于反馈超时，FreeRTOS Tick 只用于任务调度和阶段计时（两种时间不能混算）。 */
    /* 测试宏为 1 时只执行 M2006 自循环；正式 PWM Ramp 不执行，摩擦轮保留初始化停止值。
     * 自循环不读取鼠标左键；该宏只能在显式台架调试时启用。 */
#if FEED_MOTOR_TEST_ENABLE
    /* 硬件调参模式：唯一供弹电机执行非阻塞上弹/停止/下弹自循环（每次只推进一小步）。 */
    C610_M2006_TestSelfCycle_Run(&runtime.motor, &test_config, HAL_GetTick());
#else
    /*
     * 正式模式：执行供弹命令和阶段控制；测试自循环不进入此路径（两条路径不会同时写电流）。
     */
    FeedMotor_RuntimeRunCycle(&runtime, HAL_GetTick(),
                              xTaskGetTickCount());
#endif
    /* 绝对唤醒点避免一次日志或 HAL 调用耗时把 2 ms 周期逐步推迟。 */
    /* 只延迟当前任务；CAN ISR 仍更新反馈。运行时按实际 Tick 差换算 dt_ms，
     * 状态持续时间也按 Tick 差判断，不按循环次数计时。（周期变化不能改变毫秒含义。） */
    vTaskDelayUntil(&last_wake_tick,
                    pdMS_TO_TICKS(FEED_MOTOR_TASK_PERIOD_MS));
  }
#endif
}
