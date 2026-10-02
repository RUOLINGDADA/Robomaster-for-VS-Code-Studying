/**
  ******************************************************************************
  * @file    task_feed_motor.c
  * @brief   M2006 供弹电机的 FreeRTOS 自循环上弹/下弹测试任务。
  *
  * 本文件提供一个强定义的 task_feed_motor_entry()，覆盖 CubeMX 在
  * freertos.c 中生成的 __weak 空任务。任务通过 C610 CAN 驱动周期发送
  * 电流指令，并在串口输出阶段和反馈，方便上电后确认电机方向与通信。
  * CAN 接收仍在 HAL 的 FIFO 中断回调中完成，任务只负责状态机和发送。
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

#include "can.h"
#include "c610_m2006.h"
#include "usart.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * 这些参数是“裸电机测试”参数，不是最终供弹控制参数。
 * M2006 的方向由电机三相线、减速箱安装方向和电调相位共同决定；
 * 如果实机上弹/下弹方向相反，只交换下面两个电流值的正负号即可。
 */
#define FEED_MOTOR_ID 1U
#define FEED_MOTOR_PERIOD_MS 2U
#define FEED_MOTOR_UP_TIME_MS 500U
#define FEED_MOTOR_STOP_TIME_MS 500U
#define FEED_MOTOR_DOWN_TIME_MS 500U
#define FEED_MOTOR_LOG_PERIOD_MS 500U

/* 先用较小电流确认机械方向，再逐步提高；不要直接从最大电流开始。 */
#define FEED_MOTOR_UP_CURRENT_RAW 700
#define FEED_MOTOR_DOWN_CURRENT_RAW (-500)

typedef enum {
  /* 上电后的安全等待：先确认 CAN 收到本电机反馈，再允许输出。 */
  FEED_MOTOR_PHASE_WAIT_FEEDBACK = 0,
  /* 正电流测试阶段，用来验证“上弹”方向。 */
  FEED_MOTOR_PHASE_UP,
  /* 上弹结束后的零电流缓冲，避免立刻反向冲击机械结构。 */
  FEED_MOTOR_PHASE_STOP_AFTER_UP,
  /* 负电流测试阶段，用来验证“下弹”方向。 */
  FEED_MOTOR_PHASE_DOWN,
  /* 下弹结束后的零电流缓冲，避免循环边界出现突变。 */
  FEED_MOTOR_PHASE_STOP_AFTER_DOWN
} FeedMotor_PhaseTypeDef;

/*
 * 本项目只有一个供弹电机，因此不需要设备数组或任务上下文结构体。
 * 句柄保存 CAN 反馈和目标电流；下面三个 Tick 变量分别服务于阶段计时、
 * 日志限频和任务周期，它们的时间基准不同，不能合并成一个变量。
 */
static C610_M2006_HandleTypeDef g_feed_motor;
static FeedMotor_PhaseTypeDef g_feed_phase;       /* 当前测试阶段。 */
static TickType_t g_feed_phase_start_tick;        /* 当前阶段开始时刻。 */
static TickType_t g_feed_last_log_tick;           /* 上次串口日志时刻。 */

static const char *FeedMotor_PhaseName(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
    return "wait_feedback";
  case FEED_MOTOR_PHASE_UP:
    return "up";
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
    return "stop_after_up";
  case FEED_MOTOR_PHASE_DOWN:
    return "down";
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    return "stop_after_down";
  default:
    return "unknown";
  }
}

static TickType_t FeedMotor_PhaseDuration(FeedMotor_PhaseTypeDef phase) {
  /*
   * 配置宏使用毫秒，FreeRTOS 延时使用 TickType_t；两者不能直接比较。
   * pdMS_TO_TICKS() 按 configTICK_RATE_HZ 把毫秒换成系统 Tick，当前工程
   * 的 Tick 频率为 1000 Hz，因此 1 ms 通常对应 1 Tick，但代码不依赖这个
   * 巧合，换板或改配置后仍能得到正确的换算结果。
   */
  switch (phase) {
  case FEED_MOTOR_PHASE_UP:
    return pdMS_TO_TICKS(FEED_MOTOR_UP_TIME_MS);
  case FEED_MOTOR_PHASE_DOWN:
    return pdMS_TO_TICKS(FEED_MOTOR_DOWN_TIME_MS);
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    return pdMS_TO_TICKS(FEED_MOTOR_STOP_TIME_MS);
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  default:
    /* WAIT 阶段没有固定结束时间，结束条件是收到有效 CAN 反馈。 */
    return 0U;
  }
}

static void FeedMotor_SetPhase(FeedMotor_PhaseTypeDef phase,
                               TickType_t now_tick) {
  /*
   * 每次进入阶段都重新记录起点，而不是使用上一个阶段的起点。
   * 这样阶段持续时间从“真正开始输出/停止”的时刻计算，通信恢复后
   * 也不会继承掉线前已经经过的时间。
   */
  g_feed_phase = phase;
  g_feed_phase_start_tick = now_tick;

  /* 阶段切换只打印一次，避免 2 ms 任务周期持续占用 UART DMA。 */
  usart_printf("[feed_motor] phase=%s\r\n", FeedMotor_PhaseName(phase));
}

static void FeedMotor_AdvancePhase(TickType_t now_tick) {
  /*
   * 这张表就是测试状态机：上弹和下弹之间各插入一个停止阶段。
   * 如果直接从正电流跳到负电流，电机转矩会瞬间反向，可能造成齿轮、
   * 拨盘或卡弹机构冲击；停止阶段也方便观察串口反馈和确认机械运动。
   */
  switch (g_feed_phase) {
  case FEED_MOTOR_PHASE_UP:
    FeedMotor_SetPhase(FEED_MOTOR_PHASE_STOP_AFTER_UP, now_tick);
    break;
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
    FeedMotor_SetPhase(FEED_MOTOR_PHASE_DOWN, now_tick);
    break;
  case FEED_MOTOR_PHASE_DOWN:
    FeedMotor_SetPhase(FEED_MOTOR_PHASE_STOP_AFTER_DOWN, now_tick);
    break;
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
    FeedMotor_SetPhase(FEED_MOTOR_PHASE_UP, now_tick);
    break;
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  default:
    FeedMotor_SetPhase(FEED_MOTOR_PHASE_WAIT_FEEDBACK, now_tick);
    break;
  }
}

static int16_t FeedMotor_CurrentForPhase(FeedMotor_PhaseTypeDef phase) {
  switch (phase) {
  case FEED_MOTOR_PHASE_UP:
    return FEED_MOTOR_UP_CURRENT_RAW;
  case FEED_MOTOR_PHASE_DOWN:
    return FEED_MOTOR_DOWN_CURRENT_RAW;
  case FEED_MOTOR_PHASE_WAIT_FEEDBACK:
  case FEED_MOTOR_PHASE_STOP_AFTER_UP:
  case FEED_MOTOR_PHASE_STOP_AFTER_DOWN:
  default:
    return 0;
  }
}

static void FeedMotor_LogFeedback(TickType_t now_tick) {
  /*
   * 任务每 2 ms 都会运行一次，但日志只需要每 500 ms 一次。
   * 如果每次循环都调用 usart_printf，串口 DMA 会被连续占用，日志本身
   * 反而可能阻塞任务，无法反映真实控制周期。因此日志计时独立于阶段计时。
   */
  if ((now_tick - g_feed_last_log_tick) <
      pdMS_TO_TICKS(FEED_MOTOR_LOG_PERIOD_MS)) {
    return;
  }

  g_feed_last_log_tick = now_tick;
  C610_M2006_FeedbackTypeDef feedback = {0};
  if (!C610_M2006_GetFeedback(&g_feed_motor, &feedback)) {
    usart_printf("[feed_motor] feedback=invalid\r\n");
    return;
  }

  usart_printf("[feed_motor] online=%u phase=%s angle=%u speed=%d current=%d "
               "temp=%u error=%u\r\n",
               C610_M2006_IsOnline(&g_feed_motor) ? 1U : 0U,
               FeedMotor_PhaseName(g_feed_phase),
               feedback.angle_raw, feedback.speed_rpm, feedback.current_raw,
               feedback.temperature_c, feedback.error_code);
}

/**
 * @brief  接收 CAN FIFO0 中的 M2006 反馈帧。
 * @param  hcan HAL CAN 句柄。
 *
 * @note   此函数运行在 CAN 中断上下文，只取帧和更新驱动快照；不要在此
 *         处调用 usart_printf，因为串口 DMA 和格式化会拖长中断时间。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  (void)C610_M2006_RxFifoCallback(hcan, CAN_RX_FIFO0);
}

/**
 * @brief  执行 M2006 供弹电机的上弹/下弹自循环测试。
 * @param  argument FreeRTOS 任务参数，本测试未使用。
 *
 * @note   本任务假定 main() 已完成 CAN 启动、过滤器和 USART1 初始化。
 *         任务发现没有反馈时会持续发送零电流，收到反馈后才使能输出。
 */
void task_feed_motor_entry(void *argument) {
  (void)argument;

  C610_M2006_ConfigTypeDef config = {
      .hcan = &hcan1,
      .motor_id = FEED_MOTOR_ID,
      .feedback_timeout_ms = C610_M2006_FEEDBACK_TIMEOUT_MS,
  };

  if (!C610_M2006_Init(&g_feed_motor, &config)) {
    usart_printf("[feed_motor] init failed, task stopped\r\n");
    for (;;) {
      /* 初始化失败时保留任务但主动让出 CPU，避免空转占满一个优先级。 */
      vTaskDelay(pdMS_TO_TICKS(1000U));
    }
  }
    usart_printf("[feed_motor] init succ\r\n");


  /*
   * TickType_t 是 FreeRTOS 的系统时钟计数类型，不是“电机转速”或“毫秒”。
   * 它只用来比较时间间隔；真正的毫秒参数仍由上面的 *_TIME_MS 宏表达。
   */
  const TickType_t now_tick = xTaskGetTickCount();
  g_feed_last_log_tick = now_tick;
  FeedMotor_SetPhase(FEED_MOTOR_PHASE_WAIT_FEEDBACK, now_tick);

  /*
   * last_wake_tick 是周期调度锚点，与 phase_start_tick、last_log_tick 不同：
   * - phase_start_tick：阶段已经运行了多久；
   * - g_feed_last_log_tick：距离上次打印是否达到日志间隔；
   * - last_wake_tick：任务下一次应在什么时候被唤醒。
   * 三者混用会导致阶段时间、日志频率或任务周期互相干扰。
   */
  TickType_t last_wake_tick = now_tick;
  for (;;) {
    /* 本轮只读取一次 Tick，保证超时判断和日志判断使用同一时间截面。 */
    const TickType_t current_tick = xTaskGetTickCount();

    (void)C610_M2006_Process(&g_feed_motor, (uint32_t)current_tick);

    /* Process() 可能刚刚把超时设备切成 OFFLINE，所以必须在它之后读取状态。 */
    const bool online = C610_M2006_IsOnline(&g_feed_motor);

    if (!online) {
      /*
       * 离线包含“还没收到第一帧反馈”和“已有反馈但超时”两种情况。
       * 两种情况都不能继续使用上一次目标电流，所以同时关闭输出并清零。
       */
      /* 没有反馈时强制停机，防止通信中断后继续输出旧目标。 */
      (void)C610_M2006_SetOutputEnabled(&g_feed_motor, false);
      (void)C610_M2006_SetCurrent(&g_feed_motor, 0);
      if (g_feed_phase != FEED_MOTOR_PHASE_WAIT_FEEDBACK) {
        FeedMotor_SetPhase(FEED_MOTOR_PHASE_WAIT_FEEDBACK, current_tick);
      }
    } else {
      if (g_feed_phase == FEED_MOTOR_PHASE_WAIT_FEEDBACK) {
        /* 第一次收到反馈说明 CAN 总线、电机 ID 和接收路径基本可用。 */
        (void)C610_M2006_SetOutputEnabled(&g_feed_motor, true);
        FeedMotor_SetPhase(FEED_MOTOR_PHASE_UP, current_tick);
      }

      /*
       * 用“当前 Tick - 阶段起点”计算已运行时间。Tick 是无符号计数，
       * 减法会自然处理回绕；这里的阶段间隔只有几秒，远小于 Tick 的回绕周期。
       */
      const TickType_t phase_elapsed = current_tick - g_feed_phase_start_tick;
      if (phase_elapsed >= FeedMotor_PhaseDuration(g_feed_phase)) {
        FeedMotor_AdvancePhase(current_tick);
      }

      (void)C610_M2006_SetCurrent(&g_feed_motor,
                                  FeedMotor_CurrentForPhase(g_feed_phase));
    }

    /*
     * C610 电流命令是周期控制帧，不是一次设置后永久保持的寄存器。
     * 即使目标值没有变化，也必须持续发送；通信中断时驱动会把目标清零。
     */
    (void)C610_M2006_SendAll(&hcan1);
    FeedMotor_LogFeedback(current_tick);
    /*
     * vTaskDelayUntil() 使用 last_wake_tick 作为固定节拍，比 vTaskDelay()
     * 更不容易产生累计漂移：本轮计算耗时不会被重复加到下一轮周期中。
     */
    vTaskDelayUntil(&last_wake_tick,
                    pdMS_TO_TICKS(FEED_MOTOR_PERIOD_MS));
  }
}
