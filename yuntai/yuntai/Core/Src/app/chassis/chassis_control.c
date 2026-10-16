/**
 * @file chassis_control.c
 * @brief MyChassis1 普通混控和键盘直线/对数曲线的纯控制移植。
 *
 * 只由一个底盘任务调用，每轮使用本项目 PID；不解析 DBUS 或访问 CAN。
 * 模式所有权和通信安全门在任务运行时；本模块处理数值合法性与单轮反馈失效。
 */
#include "app/chassis/chassis_control.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

/* 所有可调量必须有限；先检查参数再初始化，避免 NaN 进入电流整数转换。 */
static bool ChassisControl_ConfigValid(const ChassisControl_ConfigTypeDef *c) {
  return c != NULL && isfinite(c->max_speed_rpm) && c->max_speed_rpm > 0 &&
      isfinite(c->kp) && c->kp >= 0 && isfinite(c->ki) && c->ki >= 0 &&
      isfinite(c->kd) && c->kd >= 0 && isfinite(c->integral_error_scale) && c->integral_error_scale >= 0 &&
      isfinite(c->current_limit_raw) && c->current_limit_raw > 0 && c->current_limit_raw <= 16384 &&
      isfinite(c->integral_limit_raw) && c->integral_limit_raw >= 0 && c->integral_limit_raw <= c->current_limit_raw &&
      c->stick_deadband_raw >= 0 && c->stick_deadband_raw < 660 &&
      isfinite(c->ramp_t1_s) && c->ramp_t1_s > 0 && isfinite(c->ramp_t3_s) && c->ramp_t3_s > c->ramp_t1_s &&
      isfinite(c->ramp_linear_ratio) && c->ramp_linear_ratio > 0 && c->ramp_linear_ratio < 1 &&
      isfinite(c->ramp_log_shape) && c->ramp_log_shape > 0;
}

/* 对称限幅，混控后逐轮钳位；保留源工程组合方向的饱和方式。 */
static float ChassisControl_Clamp(float value, float limit) {
  return value > limit ? limit : value < -limit ? -limit : value;
}

/* 死区沿用源工程严格小于 ±30 的判定，死区外不扣除宽度。 */
static float ChassisControl_Stick(int16_t raw, const ChassisControl_ConfigTypeDef *c) {
  if (raw > -c->stick_deadband_raw && raw < c->stick_deadband_raw) { return 0; }
  return ChassisControl_Clamp((float)raw, 660.0f) * c->max_speed_rpm / 660.0f;
}

/* 按 DJI keyboard 位图取正负方向；同轴相反键同时按下等于回中。 */
static float ChassisControl_KeyAxis(uint16_t keys, uint32_t positive_bit, uint32_t negative_bit, float limit) {
  const int positive = (keys & (1U << positive_bit)) != 0U;
  const int negative = (keys & (1U << negative_bit)) != 0U;
  return (float)(positive - negative) * limit;
}

/* 源曲线：直线至 t1，之后对数基线加端点斜率修正，t3 以零斜率进入恒速。 */
static float ChassisControl_Profile(const ChassisControl_ConfigTypeDef *c, float elapsed_s) {
  if (elapsed_s <= 0) { return 0; }
  if (elapsed_s < c->ramp_t1_s) { return c->ramp_linear_ratio * elapsed_s / c->ramp_t1_s; }
  if (elapsed_s >= c->ramp_t3_s) { return 1; }
  const float duration = c->ramp_t3_s - c->ramp_t1_s;
  const float u = (elapsed_s - c->ramp_t1_s) / duration;
  const float remaining = 1 - u;
  const float log_scale = log1pf(c->ramp_log_shape);
  const float start_slope = c->ramp_log_shape / log_scale;
  const float end_slope = start_slope / (1 + c->ramp_log_shape);
  const float join_slope = c->ramp_linear_ratio * duration / (c->ramp_t1_s * (1 - c->ramp_linear_ratio));
  float curve = log1pf(c->ramp_log_shape * u) / log_scale;
  curve += (join_slope - start_slope) * u * remaining * remaining;
  curve += end_slope * u * u * remaining;
  return fminf(1, fmaxf(c->ramp_linear_ratio, c->ramp_linear_ratio + (1 - c->ramp_linear_ratio) * curve));
}

/* 从当前连续值开始下一段，不把任务实际 dt 混成固定 2 ms。 */
static void ChassisControl_StartRamp(ChassisControl_RampAxisTypeDef *axis, float target) {
  axis->start_rpm = axis->value_rpm;
  axis->target_rpm = target;
  axis->elapsed_s = 0;
  axis->active = true;
}

/* 松键立即零目标；直接反向必须先完成零速段，零速完成周期不启动反向段。 */
static float ChassisControl_Ramp(ChassisControl_RampAxisTypeDef *axis, float desired,
                                const ChassisControl_ConfigTypeDef *c, float dt_s) {
  if (desired == 0) {
    *axis = (ChassisControl_RampAxisTypeDef){0};
    return 0;
  }
  const bool opposite = axis->value_rpm != 0 && ((axis->value_rpm > 0) != (desired > 0));
  const float next_target = opposite ? 0 : desired;
  if (!axis->active) {
    if (axis->value_rpm != desired) { ChassisControl_StartRamp(axis, next_target); }
  } else if (axis->target_rpm != next_target) {
    ChassisControl_StartRamp(axis, next_target);
  }
  if (axis->active) {
    axis->elapsed_s += dt_s;
    if (axis->elapsed_s >= c->ramp_t3_s) {
      axis->value_rpm = axis->target_rpm;
      axis->active = false;
    } else {
      axis->value_rpm = axis->start_rpm + (axis->target_rpm - axis->start_rpm) *
                        ChassisControl_Profile(c, axis->elapsed_s);
    }
  }
  return axis->value_rpm;
}

/* Init 保留四个独立 PID 对象，禁止通过共享历史耦合左右轮。 */
bool ChassisControl_Init(ChassisControl_HandleTypeDef *control, const ChassisControl_ConfigTypeDef *config) {
  if (control == NULL) { return false; }
  memset(control, 0, sizeof(*control));
  if (!ChassisControl_ConfigValid(config)) { return false; }
  control->config = *config;
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    Pid_Init(&control->speed_pid[i], config->kp, config->ki, config->kd,
             -config->current_limit_raw, config->current_limit_raw,
             -config->integral_limit_raw, config->integral_limit_raw);
    if (!Pid_ConfigureIntegral(&control->speed_pid[i], config->integral_error_scale, true)) { return false; }
  }
  control->ready = true;
  return true;
}

/* Reset 只清运行历史，模式及方向属于任务，不在这里猜测。 */
void ChassisControl_Reset(ChassisControl_HandleTypeDef *control) {
  if (control == NULL) { return; }
  memset(control->ramp, 0, sizeof(control->ramp));
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) { Pid_Reset(&control->speed_pid[i]); }
}

/* 先生成三轴目标，再混控与逐轮限幅，最后根据各轮在线门独立计算速度环。
 * 停机不运行速度环；运动模式零目标仍可输出反向电流制动。 */
bool ChassisControl_Update(ChassisControl_HandleTypeDef *control, Chassis_ModeTypeDef mode,
    int16_t vx_raw, int16_t vy_raw, uint16_t keyboard,
    const ChassisControl_FeedbackTypeDef feedback[CHASSIS_WHEEL_COUNT], float dt_s,
    ChassisControl_OutputTypeDef *output) {
  if (output == NULL) { return false; }
  memset(output, 0, sizeof(*output));
  if (control == NULL || !control->ready || feedback == NULL || !isfinite(dt_s) || dt_s <= 0 ||
      mode < CHASSIS_MODE_STOP || mode > CHASSIS_MODE_KEYBOARD) {
    ChassisControl_Reset(control);
    return false;
  }
  if (mode == CHASSIS_MODE_STOP) { ChassisControl_Reset(control); return true; }
  const ChassisControl_ConfigTypeDef *c = &control->config;
  float vx = 0, vy = 0, wz = 0;
  if (mode == CHASSIS_MODE_STICK) {
    vx = ChassisControl_Stick(vx_raw, c);
    vy = ChassisControl_Stick(vy_raw, c);
  } else {
    /* DJI bit0/1=W/S，2/3=A/D，6/7=Q/E；Ctrl 由任务模式逻辑独立处理。 */
    vx = ChassisControl_Ramp(&control->ramp[0], ChassisControl_KeyAxis(keyboard, 3, 2, c->max_speed_rpm), c, dt_s);
    vy = ChassisControl_Ramp(&control->ramp[1], ChassisControl_KeyAxis(keyboard, 0, 1, c->max_speed_rpm), c, dt_s);
    wz = ChassisControl_Ramp(&control->ramp[2], ChassisControl_KeyAxis(keyboard, 7, 6, c->max_speed_rpm), c, dt_s);
  }
  output->axis_speed_rpm[0] = vx;
  output->axis_speed_rpm[1] = vy;
  output->axis_speed_rpm[2] = wz;
  const float mix[CHASSIS_WHEEL_COUNT] = {vy - vx - wz, vy + vx + wz, vy - vx + wz, vy + vx - wz};
  for (uint32_t i = 0; i < CHASSIS_WHEEL_COUNT; ++i) {
    output->target_speed_rpm[i] = ChassisControl_Clamp(mix[i], c->max_speed_rpm);
    if (!feedback[i].online || !isfinite(feedback[i].speed_rpm)) {
      Pid_Reset(&control->speed_pid[i]);
      continue;
    }
    const float current = Pid_Update(&control->speed_pid[i], output->target_speed_rpm[i] - feedback[i].speed_rpm, dt_s);
    if (!isfinite(current)) {
      Pid_Reset(&control->speed_pid[i]);
      continue;
    }
    output->current_raw[i] = (int16_t)ChassisControl_Clamp(current, c->current_limit_raw);
  }
  return true;
}
