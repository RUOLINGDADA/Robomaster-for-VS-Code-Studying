/**
 * @file mouse_virtual_joystick.c
 * @brief DBUS 鼠标位移测速、缺帧保持和线性回中实现。
 *
 * 每帧鼠标字段是相对位移。帧序号去重后，用接收时间差计算本批速度。
 * 新速度替换旧速度，避免反向输入先抵消历史累计；新帧零位移直接清零。
 * 缺帧时 X/Y 独立保持和回中。回中使用固定起点和绝对时间，重复调度不会加快回中。
 * 输出保留浮点千分比和小增量。模块不访问外设或 RTOS。
 */
#include "algorithm/mouse_virtual_joystick/mouse_virtual_joystick.h"

#include <limits.h>
#include <math.h>
#include <stddef.h>

/* 限制已检查的浮点速度。保留小数，避免低增益的小动作在输入端丢失。 */
static float MouseVirtualJoystick_Clamp(float value, int32_t limit) {
  const float bound = (float)limit;
  return value > bound ? bound : value < -bound ? -bound : value;
}

/* 清除数值和去重历史，保留参数及 initialized；离线恢复从零接收首个新事件。 */
static void MouseVirtualJoystick_ClearState(MouseVirtualJoystick_HandleTypeDef *handle) {
  handle->x = (MouseVirtualJoystick_AxisTypeDef){0};
  handle->y = (MouseVirtualJoystick_AxisTypeDef){0};
  handle->last_frame_sequence = 0U;
  handle->last_frame_timestamp_ms = 0U;
  handle->last_update_ms = 0U;
  handle->has_frame = false;
  handle->has_update = false;
  handle->state = MOUSE_VIRTUAL_JOYSTICK_STATE_IDLE;
}

/* 无符号相减支持正常回绕；半圈以上差值无法判定先后，按无效时序处理。 */
static bool MouseVirtualJoystick_IsForward(uint32_t newer, uint32_t older) {
  return newer - older <= (uint32_t)INT32_MAX;
}

/* 按接收时刻推进本轴回中。固定起点使回中时长与任务调用次数无关。 */
static void MouseVirtualJoystick_AdvanceAxis(
    MouseVirtualJoystick_AxisTypeDef *axis,
    const MouseVirtualJoystick_AxisConfigTypeDef *config,
    uint32_t now_ms) {
  if (!axis->has_event) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_IDLE;
    return;
  }
  const uint32_t age_ms = now_ms - axis->last_event_ms;
  if (age_ms < config->hold_ms) {
    axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_HOLD;
    return;
  }
  const uint32_t elapsed_ms = age_ms - config->hold_ms;
  if (elapsed_ms >= config->decay_ms) {
    *axis = (MouseVirtualJoystick_AxisTypeDef){0};
    return;
  }
  axis->value_permille = axis->anchor_permille *
      ((float)(config->decay_ms - elapsed_ms) / (float)config->decay_ms);
  axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_DECAY;
}

/* 新帧按接收间隔测速并替换速度；重复帧只推进回中，不能把同一位移再次当作运动。 */
static bool MouseVirtualJoystick_UpdateAxis(
    MouseVirtualJoystick_AxisTypeDef *axis,
    const MouseVirtualJoystick_AxisConfigTypeDef *config,
    int32_t output_limit_permille,
    float period_scale,
    bool new_frame,
    uint32_t now_ms,
    uint32_t event_ms,
    int32_t delta_count) {
  MouseVirtualJoystick_AdvanceAxis(axis, config, now_ms);
  if (!new_frame) {
    return true;
  }
  if (delta_count == 0) {
    *axis = (MouseVirtualJoystick_AxisTypeDef){0};
    return true; /* 接收器明确报告停手时直接停止，不等待缺帧超时。 */
  }
  const float value = (float)delta_count * config->gain_permille_per_count * period_scale;
  if (!isfinite(value)) {
    return false; /* 非有限计算结果交由调用者清零，禁止再转换为整数。 */
  }
  axis->value_permille = MouseVirtualJoystick_Clamp(value, output_limit_permille);
  axis->anchor_permille = axis->value_permille;
  axis->last_event_ms = event_ms;
  axis->has_event = axis->value_permille != 0.0f;
  MouseVirtualJoystick_AdvanceAxis(axis, config, now_ms);
  axis->state = MOUSE_VIRTUAL_JOYSTICK_STATE_INPUT;
  return true;
}

/**
 * @brief 校验配置并初始化虚拟鼠标算法。
 * @param handle 算法状态存储区。
 * @param config 配置副本，时间为 ms，增益为参考周期下的 ‰/count。
 * @retval true 初始化成功；false 参数无效，有效句柄被清零并失效。
 * @note 单一任务上下文调用，不保存外部指针。
 */
bool MouseVirtualJoystick_Init(
    MouseVirtualJoystick_HandleTypeDef *handle,
    const MouseVirtualJoystick_ConfigTypeDef *config) {
  if (handle == NULL) {
    return false;
  }
  /* 先复制参数以支持 config 指向 handle 内配置；检查失败不能沿用旧有效状态。 */
  const MouseVirtualJoystick_ConfigTypeDef copy = config != NULL
      ? *config : (MouseVirtualJoystick_ConfigTypeDef){0};
  *handle = (MouseVirtualJoystick_HandleTypeDef){0};
  if (config == NULL || copy.x.hold_ms > (uint32_t)INT32_MAX || copy.x.decay_ms == 0U ||
      copy.x.decay_ms > (uint32_t)INT32_MAX - copy.x.hold_ms ||
      copy.y.hold_ms > (uint32_t)INT32_MAX || copy.y.decay_ms == 0U ||
      copy.y.decay_ms > (uint32_t)INT32_MAX - copy.y.hold_ms ||
      copy.reference_period_ms == 0U || copy.reference_period_ms > (uint32_t)INT32_MAX ||
      copy.output_limit_permille <= 0 || copy.output_limit_permille > 1000 ||
      !isfinite(copy.x.gain_permille_per_count) || !isfinite(copy.y.gain_permille_per_count)) {
    return false;
  }
  handle->config = copy;
  handle->initialized = true;
  return true;
}

/**
 * @brief 清零输出和运行历史，保留已校验配置。
 * @param handle 算法状态，允许空指针。
 * @retval None。
 * @note 仅所属任务调用；不得与 Update 并发访问。
 */
void MouseVirtualJoystick_Reset(MouseVirtualJoystick_HandleTypeDef *handle) {
  if (handle != NULL) {
    MouseVirtualJoystick_ClearState(handle);
  }
}

/**
 * @brief 消费鼠标帧并计算当前虚拟速度。
 * @param handle 算法状态；同一实例由一个任务独占。
 * @param now_ms 取得快照后的当前 HAL ms 时刻。
 * @param frame_sequence 合法帧累计序号，支持向前跳号和 uint32_t 回绕。
 * @param frame_timestamp_ms 合法帧的接收时刻，单位 HAL ms。
 * @param mouse_x 本批合法帧 X 位移代数和，单位 count；重复帧不再消费。
 * @param mouse_y 本批合法帧 Y 位移代数和，单位 count；新帧零值清零本轴。
 * @param online false 时立即清零。
 * @param virtual_x 输出 X 速度，单位 ‰。
 * @param virtual_y 输出 Y 速度，单位 ‰。
 * @retval true 更新有效；false 时非空输出写零并清除运行状态。
 * @note 纯算法；ISR 不调用，调用者不得把任务 Tick 与 HAL ms 混用。
 */
bool MouseVirtualJoystick_Update(
    MouseVirtualJoystick_HandleTypeDef *handle,
    uint32_t now_ms,
    uint32_t frame_sequence,
    uint32_t frame_timestamp_ms,
    int32_t mouse_x,
    int32_t mouse_y,
    bool online,
    float *virtual_x,
    float *virtual_y) {
  /* 先准备失败输出，再检查输入时间。无效参数不能留下旧速度。（失效立即松开虚拟杆。） */
  if (virtual_x != NULL) {
    *virtual_x = 0.0f;
  }
  if (virtual_y != NULL) {
    *virtual_y = 0.0f;
  }
  if (handle == NULL) {
    return false;
  }
  if (virtual_x == NULL || virtual_y == NULL || !handle->initialized || !online ||
      !MouseVirtualJoystick_IsForward(now_ms, frame_timestamp_ms) ||
      (handle->has_update && !MouseVirtualJoystick_IsForward(now_ms, handle->last_update_ms)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_sequence, handle->last_frame_sequence)) ||
      (handle->has_frame && !MouseVirtualJoystick_IsForward(frame_timestamp_ms, handle->last_frame_timestamp_ms)) ||
      (handle->has_frame && frame_sequence == handle->last_frame_sequence &&
       frame_timestamp_ms != handle->last_frame_timestamp_ms) ||
      (handle->x.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->x.last_event_ms)) ||
      (handle->y.has_event && !MouseVirtualJoystick_IsForward(now_ms, handle->y.last_event_ms))) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }

  const bool new_frame = !handle->has_frame || frame_sequence != handle->last_frame_sequence;
  /* 任务周期不代表鼠标采样间隔。多帧累计用前后接收时间覆盖整批，避免任务延迟放大速度。
   * 首帧和同毫秒帧没有可用间隔，使用配置参考周期，禁止除零。 */
  uint32_t period_ms = handle->has_frame
      ? frame_timestamp_ms - handle->last_frame_timestamp_ms : 0U;
  if (period_ms == 0U) {
    period_ms = handle->config.reference_period_ms;
  }
  const float period_scale = (float)handle->config.reference_period_ms / (float)period_ms;
  if (!MouseVirtualJoystick_UpdateAxis(&handle->x, &handle->config.x, handle->config.output_limit_permille,
                                       period_scale, new_frame, now_ms, frame_timestamp_ms, mouse_x) ||
      !MouseVirtualJoystick_UpdateAxis(&handle->y, &handle->config.y, handle->config.output_limit_permille,
                                       period_scale, new_frame, now_ms, frame_timestamp_ms, mouse_y)) {
    MouseVirtualJoystick_Reset(handle);
    return false;
  }
  handle->last_frame_sequence = frame_sequence;
  handle->last_frame_timestamp_ms = frame_timestamp_ms;
  handle->last_update_ms = now_ms;
  handle->has_frame = true;
  handle->has_update = true;
  handle->state = handle->x.state > handle->y.state ? handle->x.state : handle->y.state;
  /* 状态已限幅且有限。保留小数，避免低增益输入在命令边界丢失。 */
  *virtual_x = handle->x.value_permille;
  *virtual_y = handle->y.value_permille;
  return true;
}
