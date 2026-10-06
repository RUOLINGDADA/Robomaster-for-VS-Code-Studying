/**
 * @file gimbal_control.c
 * @brief 云台角度环、速度 PID、重力补偿与电流 Ramp。
 *
 * 角度使用连续 count，速度使用 rpm，输出使用 GM6020 电流原始值。
 * 每轴由所属任务独占实例。输入反馈必须来自同一份 CAN 快照。
 * 本模块只计算输出，不发送 CAN，不检查反馈年龄或配置合法性。
 * 运行时先检查安全门；边界只过滤向外命令，反向命令仍可离开边界。
 */

#include "app/gimbal/gimbal_control.h"

#include <math.h>
#include <stddef.h>

/* 限制目标、积分和合成电流；各项使用调用者指定的单位和范围。 */
static float GimbalControl_Clamp(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

/* 将浮点控制结果转换为 CAN 电流整数；先饱和再转换，避免浮点越界折返成反向大电流。 */
static int16_t GimbalControl_ToCurrent(float current_raw) {
  if (current_raw > 32767.0f) {
    return 32767;
  }
  if (current_raw < -32768.0f) {
    return -32768;
  }
  return (int16_t)current_raw;
}

/**
 * @brief  初始化角度/速度级联控制器及其滤波、Ramp 状态（建立控制历史的起点）。
 * @param  controller 调用者持有的控制器对象（每个云台轴独占一个）。
 * @param  config 控制增益、范围、速度和电流变化率参数（单位必须与反馈一致）。
 * @param  initial_angle_raw 第一次有效反馈的连续角度，单位原始计数（作为初始保持位置）。
 * @note   正式轴运行时先校验增益非负/有限、积分上限不超过总电流、滤波权重在 (0,1]（直接调用者也须满足这些前提）。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_Init(GimbalControl_HandleTypeDef *controller,
                     const GimbalControl_ConfigTypeDef *config,
                     int32_t initial_angle_raw) {
  if (controller == NULL) {
    return;
  }
  /* 先撤销旧初始化状态，再建立新控制历史；失败后不能沿用旧配置输出电流。 */
  *controller = (GimbalControl_HandleTypeDef){0};
  if (config == NULL ||
      config->min_angle_raw >= config->max_angle_raw ||
      !isfinite(config->max_current_raw) ||
      config->max_current_raw <= 0.0f) {
    return;
  }

  controller->config = *config;
  controller->center_angle_raw = config->center_angle_raw;
  controller->target_angle_raw = (float)initial_angle_raw;
  controller->position_integral_rpm = 0.0f;
  controller->target_speed_rpm = 0.0f;
  Pid_Init(&controller->speed_pid, config->speed_kp_current_per_rpm,
           config->speed_ki_current_per_rpm_s, config->speed_kd_current_s_per_rpm,
           -config->max_current_raw, config->max_current_raw,
           -config->speed_integral_limit_raw, config->speed_integral_limit_raw);
  Ramp_Init(&controller->current_ramp, 0.0f);
  LowPassFilter_Init(&controller->speed_filter, 0.0f,
                     config->speed_filter_alpha);
  controller->initialized = GravityCompensation_Init(
      &controller->gravity_compensation, &config->gravity_compensation);
}

/**
 * @brief  清除 PID、滤波和 Ramp 历史，保留最后目标位置。
 * @param  controller 已初始化的通用云台控制器（不能与另一轴共用）。
 * @note   仅所属任务调用。反馈恢复后从零电流重新爬坡，目标不会跟随反馈改写。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 */
void GimbalControl_ResetHistory(GimbalControl_HandleTypeDef *controller) {
  if (controller == NULL || !controller->initialized) {
    return;
  }
  controller->position_integral_rpm = 0.0f;
  controller->target_speed_rpm = 0.0f;
  Pid_Reset(&controller->speed_pid);
  Ramp_Reset(&controller->current_ramp, 0.0f);
  LowPassFilter_Reset(&controller->speed_filter, 0.0f);
}

/**
 * @brief  设置一个固定保持目标，不改变 PID、Ramp 和滤波历史（测试角度环时使用）。
 * @param  controller 已初始化的通用云台控制器（调用者独占）。
 * @param  target_angle_raw 目标连续角度，函数会钳位到配置范围（越界不会继续向外驱动）。
 * @retval true 设置成功；false 参数为空或控制器尚未初始化。
 * @note   适用于硬件角度保持调参；正式相对速度模式仍通过速度命令改变目标（两种模式复用同一控制器）。
 */
bool GimbalControl_SetTargetAngle(GimbalControl_HandleTypeDef *controller,
                               int32_t target_angle_raw) {
  if (controller == NULL || !controller->initialized) {
    return false;
  }

  controller->target_angle_raw = GimbalControl_Clamp(
      (float)target_angle_raw, (float)controller->config.min_angle_raw,
      (float)controller->config.max_angle_raw);
  return true;
}

/**
 * @brief  运行位置 PID、速度 PID、重力补偿与统一电流 Ramp。
 * @param  controller 通用云台控制器（函数会更新 PID、滤波和 Ramp 历史）。
 * @param  feedback 同一次 CAN 快照中的反馈（角度和速度必须来自同一帧）。
 * @param  command 已完成超时检查的遥控或固定目标命令（固定目标标记避免零速度被当成回中）。
 * @param  dt_s 控制周期，单位秒（用于把 rpm 换算成这段时间内的角度变化）。
 * @param  output 输出的目标角度、速度和电流（调用者随后交给驱动）。
 * @retval None 输出结果写入对象或参数，函数无返回值。
 * @note 仅所属任务串行调用；同一实例不能并发修改。
 */
void GimbalControl_Update(GimbalControl_HandleTypeDef *controller,
                          const Gm6020_FeedbackTypeDef *feedback,
                          const Gimbal_CommandTypeDef *command,
                          float dt_s,
                          GimbalControl_OutputTypeDef *output) {
  if (output != NULL) {
    *output = (GimbalControl_OutputTypeDef){0};
  }
  if (controller == NULL || feedback == NULL || command == NULL ||
      output == NULL || !controller->initialized || !isfinite(dt_s) || dt_s <= 0.0f) {
    return;
  }

  const float filtered_speed =
      LowPassFilter_Update(&controller->speed_filter,
                           (float)feedback->speed_rpm);
  const float requested_command_speed = command->enabled
      ? ((float)command->velocity_permille / 1000.0f) *
            controller->config.max_command_speed_rpm
      : 0.0f;
  const bool at_min = feedback->angle_total_raw <= controller->config.min_angle_raw;
  const bool at_max = feedback->angle_total_raw >= controller->config.max_angle_raw;
  const bool remote_command = !command->fixed_target;
  const bool outward_at_min = remote_command && at_min &&
                              requested_command_speed < 0.0f;
  const bool outward_at_max = remote_command && at_max &&
                              requested_command_speed > 0.0f;
  /* 软件边界必须同时屏蔽目标积分和速度前馈。只拦截目标积分会留下正的
   * 前馈速度，位置误差虽然为零，速度环仍会向挡块输出电流。（边界像红灯，
   * 不能只禁止加速目标，却继续把油门信号送进下一环。） */
  const bool block_outward = outward_at_min || outward_at_max;
  const float command_speed = block_outward ? 0.0f : requested_command_speed;
  const bool at_or_beyond = at_min || at_max;

  /* 只有非零有效输入更新目标。边界钳位也只作用于本次积分；零输入不能改写保存目标。
   * 越界反馈不触发重定位，向内命令从最后目标连续积分。（松手只停止目标移动。） */
  if (command_speed != 0.0f) {
    controller->target_angle_raw +=
        command_speed * ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f) * dt_s;
    if (!at_or_beyond) {
      controller->target_angle_raw = GimbalControl_Clamp(
          controller->target_angle_raw, (float)controller->config.min_angle_raw,
          (float)controller->config.max_angle_raw);
    }
  }

  const float angle_error = controller->target_angle_raw -
                            (float)feedback->angle_total_raw;
  const float previous_integral = controller->position_integral_rpm;
  const float integral_candidate = previous_integral +
      controller->config.position_ki_rpm_per_raw_s * angle_error * dt_s;
  controller->position_integral_rpm = GimbalControl_Clamp(
      integral_candidate, -controller->config.position_integral_limit_rpm,
      controller->config.position_integral_limit_rpm);
  const float angle_error_rate_raw_per_s =
      (command_speed - filtered_speed) *
      ((float)GM6020_ENCODER_COUNTS_PER_REV / 60.0f);
  const float target_speed_unclamped =
      controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  if ((target_speed_unclamped > controller->config.max_speed_target_rpm &&
       angle_error > 0.0f) ||
      (target_speed_unclamped < -controller->config.max_speed_target_rpm &&
       angle_error < 0.0f)) {
    /* 恢复上一周期的限幅积分。不能从已钳位值减完整增量，否则大误差会造成反向越限。 */
    controller->position_integral_rpm = previous_integral;
  }
  float target_speed = controller->config.position_kp_rpm_per_raw * angle_error +
      controller->position_integral_rpm +
      controller->config.position_kd_rpm_s_per_raw * angle_error_rate_raw_per_s +
      controller->config.velocity_feedforward_gain * command_speed;
  target_speed = GimbalControl_Clamp(target_speed,
                                     -controller->config.max_speed_target_rpm,
                                     controller->config.max_speed_target_rpm);
  controller->target_speed_rpm = target_speed;

  const float speed_error = target_speed - filtered_speed;
  const float speed_loop_current =
      Pid_Update(&controller->speed_pid, speed_error, dt_s);
  const float gravity_current = GravityCompensation_Update(
      &controller->gravity_compensation, feedback->angle_total_raw);
  const float combined_current = GimbalControl_Clamp(
      speed_loop_current + gravity_current, -controller->config.max_current_raw,
      controller->config.max_current_raw);
  if (!isfinite(combined_current)) {
    GimbalControl_ResetHistory(controller);
    return; /* 非有限计算结果不转换为 CAN 整数；输出保持本周期初始零值。 */
  }
  const float ramped_current = Ramp_Update(
      &controller->current_ramp, combined_current,
      controller->config.current_slew_raw_per_s, dt_s);

  output->target_angle_raw = (int32_t)controller->target_angle_raw;
  output->angle_error_raw = (int32_t)angle_error;
  output->target_speed_rpm = target_speed;
  output->speed_loop_current_raw = GimbalControl_ToCurrent(speed_loop_current);
  output->gravity_compensation_current_raw =
      GimbalControl_ToCurrent(gravity_current);
  output->target_current_raw = GimbalControl_ToCurrent(ramped_current);
  output->at_min_limit = at_min;
  output->at_max_limit = at_max;
}
