/**
 * @file gravity_compensation.c
 * @brief 连续角度余弦重力补偿的纯计算实现。
 *
 * 本文件不修改目标位置，不读取反馈，不发送电流。
 * 任务先取得一致 GM6020 快照，再传入连续角度；反馈失效时由上层跳过本模块。
 */

#include "algorithm/gravity_compensation/gravity_compensation.h"

#include <math.h>
#include <stddef.h>

#define GRAVITY_COMPENSATION_TWO_PI 6.28318530717958647692f /* 一圈弧度，rad；用于 8192 count/rev 的角度换算。 */
#define GRAVITY_COMPENSATION_COUNTS_PER_REV 8192LL /* GM6020 编码器一圈 8192 count；算法不依赖 CAN 驱动。 */

/* 检查配置边界，避免 NaN 或过大的补偿绕过最终电流限幅。 */
static bool GravityCompensation_ConfigIsValid(
    const GravityCompensation_ConfigTypeDef *config) {
  if (config == NULL || (config->current_sign != 1 && config->current_sign != -1) ||
      !isfinite(config->bias_current_raw) ||
      !isfinite(config->amplitude_current_raw) ||
      !isfinite(config->max_current_raw) ||
      config->bias_current_raw < 0.0f ||
      config->amplitude_current_raw < 0.0f ||
      config->max_current_raw <= 0.0f ||
      config->bias_current_raw + config->amplitude_current_raw >
          config->max_current_raw) {
    return false;
  }
  return true;
}

/* 将补偿结果限制到配置的绝对 raw 范围；先限幅再返回，避免转换后折返成反向大电流。 */
static float GravityCompensation_Clamp(float value, float maximum) {
  if (value > maximum) {
    return maximum;
  }
  if (value < -maximum) {
    return -maximum;
  }
  return value;
}

/**
 * @brief 复制并校验每轴重力模型。
 * @param handle 所属任务独占的算法对象；空指针返回 false。
 * @param config 有限 raw 参数、中心 count、方向 ±1 和开关。
 * @retval true 初始化成功；false 配置非法，对象清为未初始化。
 * @note 普通任务初始化调用；不访问硬件，不保留配置指针。
 */
bool GravityCompensation_Init(
    GravityCompensation_HandleTypeDef *handle,
    const GravityCompensation_ConfigTypeDef *config) {
  if (handle == NULL || !GravityCompensation_ConfigIsValid(config)) {
    if (handle != NULL) {
      *handle = (GravityCompensation_HandleTypeDef){0};
    }
    return false;
  }
  handle->config = *config;
  handle->initialized = true;
  return true;
}

/**
 * @brief 根据当前连续角度计算有限且限幅的补偿电流。
 * @param handle 已初始化且由所属任务独占的模型对象。
 * @param angle_total_raw 同周期反馈的连续 count。
 * @retval 补偿电流 raw；对象无效、禁用或结果非有限时返回 0。
 * @note 串行任务调用。调用者检查反馈新鲜度，再将补偿与闭环输出合成。
 */
float GravityCompensation_Update(
    GravityCompensation_HandleTypeDef *handle,
    int32_t angle_total_raw) {
  if (handle == NULL || !handle->initialized || !handle->config.enabled) {
    return 0.0f;
  }

  /* 先用 int64 求差，再按一圈取余。余弦周期不变；大连续计数转 float 后相减会丢失小角差。
   * 取余只用于重力模型，不修改传入角度或控制目标。（跨圈仍计算同一机械角的补偿。） */
  const int64_t offset_raw = (int64_t)angle_total_raw - handle->config.center_angle_raw;
  const float angle_offset_rad = (float)(offset_raw % GRAVITY_COMPENSATION_COUNTS_PER_REV) *
      GRAVITY_COMPENSATION_TWO_PI / (float)GRAVITY_COMPENSATION_COUNTS_PER_REV;
  const float magnitude = handle->config.bias_current_raw +
      handle->config.amplitude_current_raw * cosf(angle_offset_rad);
  const float current_raw = magnitude * (float)handle->config.current_sign;
  if (!isfinite(current_raw)) {
    return 0.0f;
  }
  return GravityCompensation_Clamp(current_raw, handle->config.max_current_raw);
}
