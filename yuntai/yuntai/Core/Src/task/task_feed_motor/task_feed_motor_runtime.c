/**
 * @file task_feed_motor_runtime.c
 * @brief 双 C615 PWM 与唯一 M2006 自动供弹运行时。
 *
 * CAN1 ID1 M2006 与 PE9/PE11 双 C615。
 * 仅供弹任务调用。两个 PWM 句柄由该任务独占，CAN ISR 只更新反馈。
 * 部分初始化失败时保留已注册 CAN 句柄，重试未启动 PWM 并保持停止值。
 * 每周期在短临界区复制首帧标志和反馈；控制和日志共用这一份快照。
 * 命令/反馈年龄使用 HAL ms；阶段使用 FreeRTOS Tick；Ramp 周期换成 ms。
 * 故障立即写停止脉宽。在线释放时摩擦轮 Ramp 停止，M2006 当周期清零。
 * HAL 接受 CAN 帧不等于电调执行。本模块不检测卡弹或计算实发弹数。
 */
#include "FreeRTOS.h"
#include "task.h"
#include "task/task_feed_motor/task_feed_motor_runtime.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include "can.h"
#include "tim.h"
#include "app/log/log.h"
#include <stddef.h>

#define FEED_MOTOR_COUNT_TEXT_SIZE 24U /* int64_t 十进制文本最大长度，含符号和 NUL。 */

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE
/**
 * @brief 将供弹连续角度或目标 count 转成十进制文本。
 * @param value 需要输出的有符号电机轴 count。
 * @param text 输出缓冲区，至少 FEED_MOTOR_COUNT_TEXT_SIZE 字节。
 * @retval None。手工转换避免目标端 printf 对 %lld 的支持差异。
 */
static void FeedMotorRuntime_FormatCount(
    int64_t value, char text[FEED_MOTOR_COUNT_TEXT_SIZE]) {
  char reverse[FEED_MOTOR_COUNT_TEXT_SIZE] = {0};
  uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1U : (uint64_t)value;
  size_t length = 0U;
  do {
    reverse[length++] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
  } while (magnitude != 0U && length < sizeof(reverse) - 1U);
  if (value < 0 && length < sizeof(reverse) - 1U) {
    reverse[length++] = '-';
  }
  for (size_t index = 0U; index < length; index++) {
    text[index] = reverse[length - index - 1U];
  }
  text[length] = '\0';
}
#endif

/* 周期不能换算为 0 Tick；Ramp 时间不能为 0，活动脉宽必须在停止和最大值之间。 */
_Static_assert(pdMS_TO_TICKS(FEED_MOTOR_TASK_PERIOD_MS) > 0U, "feed period must be at least one tick");
_Static_assert(FEED_MOTOR_SNAIL_RAMP_TIME_MS > 0U &&
               FEED_MOTOR_SNAIL_RAMP_TIME_MS <= INT32_MAX &&
               (FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN == 1 ||
                FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN == -1) &&
               (FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN == 1 ||
                FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN == -1) &&
               FEED_MOTOR_SNAIL_STOP_PULSE_US >= SNAIL_2305_MIN_PROTOCOL_PULSE_US &&
               FEED_MOTOR_SNAIL_MAX_PULSE_US <= SNAIL_2305_MAX_PROTOCOL_PULSE_US &&
               FEED_MOTOR_SNAIL_ACTIVE_PULSE_US <= FEED_MOTOR_SNAIL_MAX_PULSE_US &&
               FEED_MOTOR_SNAIL_ACTIVE_PULSE_US > FEED_MOTOR_SNAIL_STOP_PULSE_US,
               "invalid wheel ramp/pulse");

/* 任务私有：分别向已启动的左右通道写停止 CCR，并重置脉宽 Ramp；未启动通道返回失败。
 * 不停止 TIM1 计数，电调仍收到停止 PWM。CCR 预装载最迟下一周期生效，软件调用不证明轮已停。
 * 在线松键由主循环 Ramp 减速；故障路径不等待这段减速。（先取消油门，不等轮子转慢。） */
static void FeedMotorRuntime_StopWheels(FeedMotor_RuntimeTypeDef *runtime) {
  (void)Snail2305_Stop(&runtime->left);
  (void)Snail2305_Stop(&runtime->right);
}

/**
 * @brief 初始化三个驱动并保持停止；允许失败后重试。
 * @param runtime 静态零初始化的任务对象；不能复制到别的任务。
 * @retval true 三个驱动就绪；false 部分失败，已启动的输出仍停止。
 * @note 仅供弹任务调用；重试保留已注册的 C610 句柄。
 */
bool FeedMotor_RuntimeInit(FeedMotor_RuntimeTypeDef *runtime) {
  if (runtime == NULL) {
    return false;
  }
  /* 首次才清空对象。CAN 注册表保存句柄地址；每次重试 memset 会破坏已注册状态。
   * 后续逐个初始化并尝试清零所有成功通道，任一失败都不进入发射循环。 */
  if (!runtime->configured) {
    *runtime = (FeedMotor_RuntimeTypeDef){0};
    FeedMotorControl_Init(&runtime->control);
    runtime->configured = true;
  }
  /* M2006 使用 CAN1 电调 ID1，反馈 0x201、控制 0x200 的第一个槽位。
   * 不在这里设置 CAN 过滤器或启动总线，这些操作由 main 的 USER CODE 完成。 */
  const C610_M2006_ConfigTypeDef can_config = {
    .hcan = &hcan1, .motor_id = FEED_MOTOR_ID,
    .feedback_timeout_ms = C610_M2006_FEEDBACK_TIMEOUT_MS,
  };
  /* TIM1 的 1 MHz/50 Hz 时基来自 .ioc；驱动只检查而不覆盖配置。
   * (ACTIVE-STOP) us×1000/RAMP_MS 得到 us/s；直接用 ms 当 s 会慢 1000 倍。 */
  const Snail2305_ConfigTypeDef left_config = {
    .htim = &htim1, .channel = TIM_CHANNEL_1,
    .direction_sign = FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN,
    .stop_pulse_us = FEED_MOTOR_SNAIL_STOP_PULSE_US,
    .max_pulse_us = FEED_MOTOR_SNAIL_MAX_PULSE_US,
    .ramp_us_per_s = (float)(FEED_MOTOR_SNAIL_ACTIVE_PULSE_US - FEED_MOTOR_SNAIL_STOP_PULSE_US) *
        1000.0f / FEED_MOTOR_SNAIL_RAMP_TIME_MS,
  };
  Snail2305_ConfigTypeDef right_config = left_config;
  right_config.channel = TIM_CHANNEL_2; /* 右轮 PE11 使用独立 CCR2；保持相同目标，旋转方向由电调/相线设置。 */
  right_config.direction_sign = FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN;
  /* 不重复注册 C610。部分 PWM 失败时保留句柄，下一轮只重试未启动的通道。 */
  const bool motor_ok = runtime->motor.initialized || C610_M2006_Init(&runtime->motor, &can_config);
  const bool left_ok = Snail2305_Init(&runtime->left, &left_config);
  const bool right_ok = Snail2305_Init(&runtime->right, &right_config);
  /* 先关闭许可，再写零并尝试提交。HAL 邮箱失败仍保留零缓存，不能报告电调已执行。
   * 三个初始化调用各自执行，避免一通道失败跳过另一通道的停止配置。 */
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, false);
  (void)C610_M2006_SetCurrent(&runtime->motor, 0);
  if (motor_ok) {
    (void)C610_M2006_SendAll(&hcan1);
  }
  FeedMotorRuntime_StopWheels(runtime);
  /* 只有全部驱动成功才返回 true；失败仍保留 configured 及成功句柄，入口限频重试。 */
  runtime->initialized = motor_ok && left_ok && right_ok;
  return runtime->initialized;
}



/**
 * @brief 执行一个反馈、控制、PWM、CAN 和日志周期。
 * @param runtime 已初始化任务对象。
 * @param now_ms HAL_GetTick 时间，ms；用于命令/反馈新鲜度。
 * @param now_tick FreeRTOS Tick；用于阶段和日志。
 * @retval None；许可失效时输出停止值，不等待硬件。
 * @note 仅供弹任务调用；日志繁忙不阻塞控制。
 */
void FeedMotor_RuntimeRunCycle(FeedMotor_RuntimeTypeDef *runtime, uint32_t now_ms, TickType_t now_tick) {
  /* 初始化许可先于任何硬件访问；失败对象不能执行周期。
   * 这里直接返回，停止输出由初始化路径负责，并不再次向未启动通道写寄存器。 */
  if (runtime == NULL || !runtime->initialized) {
    return;
  }
  /* Tick 差值只算周期，再按 configTICK_RATE_HZ 换成 ms。
   * 命令与反馈年龄另用 HAL ms；即使当前都是 1 kHz 也不能直接相减。 */
  const TickType_t elapsed_tick = now_tick - runtime->last_cycle_tick;
  uint32_t dt_ms = runtime->cycle_started ?
      (uint32_t)((uint64_t)elapsed_tick * 1000U / configTICK_RATE_HZ) : FEED_MOTOR_TASK_PERIOD_MS;
  runtime->cycle_started = true;
  runtime->last_cycle_tick = now_tick;
  /* 首周期用配置时长。整 Tick 换算得到 0 ms 时也用配置时长，避免 PID/Ramp 拒绝周期。
   * 64 位乘法保存 elapsed_tick×1000 的中间值，不能先用 32 位乘法再强转。 */
  if (dt_ms == 0U) {
    dt_ms = FEED_MOTOR_TASK_PERIOD_MS;
  }
  /* 命令先零初始化；从未收到命令时保持禁用。反馈先 Process 再复制，
   * 快照成功也必须有首帧标志，不能把全零启动内存当成电机当前位置。 */
  FeedMotor_CommandTypeDef command = {0};
  (void)FeedMotorCommand_GetSnapshot(now_ms, &command);
  (void)C610_M2006_Process(&runtime->motor, now_ms);
  C610_M2006_FeedbackTypeDef feedback = {0};
  /* 在同一短临界区复制首帧标志与全部反馈。64 位计数不能逐字段直接读取（只用这一份）。 */
  taskENTER_CRITICAL();
  const bool valid = C610_M2006_GetFeedback(&runtime->motor, &feedback) && runtime->motor.feedback_received;
  taskEXIT_CRITICAL();
  /* HAL ms 无符号差允许计数回绕。ISR 可能在取 now_ms 后更新出略新的反馈，
   * 大于半周期的下溢按 0 ms 处理；valid=false 仍不能在线，0 ms 时间戳本身不表示离线。 */
  uint32_t age_ms = now_ms - feedback.last_feedback_tick;
  if (age_ms > INT32_MAX) {
    age_ms = 0U;
  }
  const bool online = valid && age_ms < C610_M2006_FEEDBACK_TIMEOUT_MS;
  /* 命令许可与反馈新鲜度共同决定输出。首发先在 FRIC_SPINUP 预旋；进入
   * FEED/FEED_SETTLE 后继续保持两路活动 PWM，避免 C610 上弹时摩擦轮带载起动。
   * 按钮释放不能中断进行中的单发；连发释放当周期清零 C610，并让 C615 目标转为停止值。 */
  const bool permitted = command.enabled && online;
  bool wheels_active = permitted && FeedMotorControl_WheelsActive(&runtime->control);
  if (runtime->control.phase == FEED_MOTOR_PHASE_CONTINUOUS_FEED &&
      !command.fire_requested) {
    wheels_active = false;
  }
  if (!permitted) {
    FeedMotorRuntime_StopWheels(runtime);
  } else {
    const uint16_t target = wheels_active ? FEED_MOTOR_SNAIL_ACTIVE_PULSE_US :
        FEED_MOTOR_SNAIL_STOP_PULSE_US;
    (void)Snail2305_SetTargetPulse(&runtime->left, target);
    (void)Snail2305_SetTargetPulse(&runtime->right, target);
    (void)Snail2305_Process(&runtime->left, dt_ms);
    (void)Snail2305_Process(&runtime->right, dt_ms);
  }
  /* PWM 到达活动脉宽只表示 CCR 已写入目标，不是 C615 实际转速反馈。 */
  const bool wheels_ready = runtime->left.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US &&
      runtime->right.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US;
  const int64_t angle_count = feedback.angle_total_raw * FEED_MOTOR_FEEDBACK_SIGN;
  const int16_t current_raw = FeedMotorControl_Update(&runtime->control, permitted,
      command.fire_requested, wheels_ready, angle_count, now_tick, dt_ms);
  /* 每周期都更新许可和电流，再提交整组 CAN 槽位；反馈失效或阶段停止仍发零。
   * 当前电流非零才允许 C610 输出，单发释放后的 FEED 阶段因此仍可继续完成。 */
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, current_raw != 0);
  (void)C610_M2006_SetCurrent(&runtime->motor, current_raw);
  const bool submitted = C610_M2006_SendAll(&hcan1);
  (void)submitted; /* 关闭日志时仍须提交电流；不能把有硬件副作用的调用放入日志宏。 */
  /* 只打印本周期快照；DMA 忙时保留提交时刻，下周期重试，不缓存旧日志。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE
  if ((TickType_t)(now_tick - runtime->last_log_tick) >= pdMS_TO_TICKS(FEED_MOTOR_LOG_PERIOD_MS)) {
    char angle_count_text[FEED_MOTOR_COUNT_TEXT_SIZE];
    FeedMotorRuntime_FormatCount(angle_count, angle_count_text);
    if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 阶段=%s 左键=%u DBUS许可=%u 反馈有效=%u 在线=%u 年龄(ms)=%lu 角度=%s 转速(rpm)=%d 计算电流=%d 输出许可=%u CAN提交=%u 左PWM(us)=%u 右PWM(us)=%u 停滞窗口(count)=%u 停滞确认(ms)=%lu\r\n",
      FeedMotorControl_PhaseName(runtime->control.phase), command.fire_requested ? 1U : 0U,
      command.enabled ? 1U : 0U, valid ? 1U : 0U, online ? 1U : 0U,
      (unsigned long)age_ms, angle_count_text,
      (int)(feedback.speed_rpm * FEED_MOTOR_FEEDBACK_SIGN), (int)current_raw,
      current_raw != 0 ? 1U : 0U,
      submitted ? 1U : 0U, (unsigned)runtime->left.pulse_us,
      (unsigned)runtime->right.pulse_us, (unsigned)FEED_MOTOR_STALL_COUNTS,
      (unsigned long)runtime->control.stall_elapsed_ms)) {
      runtime->last_log_tick = now_tick;
    }
  }
#endif
}
