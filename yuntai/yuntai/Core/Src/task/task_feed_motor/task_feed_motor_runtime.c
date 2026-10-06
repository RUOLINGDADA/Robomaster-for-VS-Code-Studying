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
  /* 命令许可与反馈新鲜度共同决定输出。故障立即停轮；在线松键只让摩擦轮缓降。
   * FeedMotorControl_Update 收到 fire=false 会当周期清零，不能等摩擦轮降速完再停拨弹。 */
  const bool permitted = command.enabled && online;
  const bool fire = permitted && command.fire_requested;
  if (!permitted) {
    FeedMotorRuntime_StopWheels(runtime);
  } else {
    /* 在线松键与正常启动都走非阻塞 Ramp；每周期只写目标和推进一步，不等待 PWM 周期。 */
    const uint16_t target = fire ? FEED_MOTOR_SNAIL_ACTIVE_PULSE_US : FEED_MOTOR_SNAIL_STOP_PULSE_US;
    (void)Snail2305_SetTargetPulse(&runtime->left, target);
    (void)Snail2305_SetTargetPulse(&runtime->right, target);
    (void)Snail2305_Process(&runtime->left, dt_ms);
    (void)Snail2305_Process(&runtime->right, dt_ms);
  }
  /* 比较两份最近 CCR 写值，任一轮还在爬坡就不允许首发；不是读取真实转速。
   * 反馈坐标符号只换角度/速度，电流符号在控制器输出处单独转换。 */
  const bool wheels_ready = runtime->left.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US &&
      runtime->right.pulse_us == FEED_MOTOR_SNAIL_ACTIVE_PULSE_US;
  const int64_t angle_count = feedback.angle_total_raw * FEED_MOTOR_FEEDBACK_SIGN;
  const int32_t speed_rpm = (int32_t)feedback.speed_rpm * FEED_MOTOR_FEEDBACK_SIGN;
  const int16_t current_raw = FeedMotorControl_Update(&runtime->control, fire, wheels_ready,
      angle_count, speed_rpm, now_tick, dt_ms);
  /* 每周期都更新许可和电流，再提交整组 CAN 槽位；fire=false 的周期仍必须发零。
   * 驱动提交前再查反馈年龄，堵住计算完成后反馈过期的窗口；HAL 失败不重试忙等。 */
  (void)C610_M2006_SetOutputEnabled(&runtime->motor, fire);
  (void)C610_M2006_SetCurrent(&runtime->motor, current_raw);
  const bool submitted = C610_M2006_SendAll(&hcan1);
  (void)submitted; /* 关闭日志时仍须提交电流；不能把有硬件副作用的调用放入日志宏。 */
  /* 只打印本周期快照；DMA 忙时保留提交时刻，下周期重试，不缓存旧日志。 */
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE && LOG_TASK_ENABLE && LOG_FEED_MOTOR_ENABLE
  if ((TickType_t)(now_tick - runtime->last_log_tick) >= pdMS_TO_TICKS(FEED_MOTOR_LOG_PERIOD_MS)) {
    if (LOG_TRY_PRINTF(LOG_CATEGORY_FEED_MOTOR, "[供弹] 阶段=%s DBUS许可=%u 反馈有效=%u 在线=%u 年龄(ms)=%lu 角度=%lld 目标=%lld 转速(rpm)=%d 计算电流=%d 输出许可=%u CAN提交=%u 左PWM(us)=%u 右PWM(us)=%u 完成步数=%lu\r\n",
      FeedMotorControl_PhaseName(runtime->control.phase), command.enabled ? 1U : 0U,
      valid ? 1U : 0U, online ? 1U : 0U, (unsigned long)age_ms,
      (long long)feedback.angle_total_raw, (long long)runtime->control.target_count,
      (int)feedback.speed_rpm, (int)current_raw, fire ? 1U : 0U,
      submitted ? 1U : 0U, (unsigned)runtime->left.pulse_us,
      (unsigned)runtime->right.pulse_us, (unsigned long)runtime->control.completed_steps)) {
      runtime->last_log_tick = now_tick;
    }
  }
#endif
}
