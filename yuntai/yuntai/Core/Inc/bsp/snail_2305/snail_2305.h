/**
 * @file snail_2305.h
 * @brief C615/Snail 2305 的双通道 PWM 与脉宽 Ramp。
 *
 * C615 V1.0 手册允许 400~2200 us，最高 500 Hz；本工程使用 50 Hz。
 * PE9/TIM1_CH1 与 PE11/TIM1_CH2 使用 1 MHz 计数、20000 us 周期。
 * 每通道由所属任务独占。先写停止 CCR，再启动 PWM；ISR 不调用本接口。
 * 驱动不修改 CubeMX 时基，不报告电调在线或实际转速。方向由电调或相线设置。
 */
#ifndef SNAIL_2305_H
#define SNAIL_2305_H /* 防止重复包含。 */

#include "stm32f4xx_hal.h"
#include "algorithm/ramp/ramp.h"
#include <stdbool.h>
#include <stdint.h>

#define SNAIL_2305_MIN_PROTOCOL_PULSE_US 400U /* C615 协议最小脉宽，us；超出范围拒绝输出。 */
#define SNAIL_2305_MAX_PROTOCOL_PULSE_US 2200U /* C615 协议最大脉宽，us；超出范围拒绝输出。 */

typedef struct {
  TIM_HandleTypeDef *htim; /* CubeMX TIM1 句柄。必须先完成 MX_TIM1_Init。 */
  uint32_t channel; /* TIM_CHANNEL_1 或 TIM_CHANNEL_2。不能传 GPIO 编号。 */
  int8_t direction_sign; /* 该通道期望的物理转向，+1/-1；C615 需由相线或 DJI Assistant 落实。 */
  uint16_t stop_pulse_us; /* 电调校准停止脉宽，us。初始化先写此值。 */
  uint16_t max_pulse_us; /* 电调校准最大脉宽，us。必须大于停止值。 */
  float ramp_us_per_s; /* 非阻塞斜坡速率，us/s。必须为有限正值。 */
} Snail2305_ConfigTypeDef;

typedef struct {
  Snail2305_ConfigTypeDef config; /* 任务持有的配置副本。外设指针不转移所有权。 */
  Ramp_HandleTypeDef ramp; /* 任务独占的脉宽斜坡历史，单位 us。 */
  uint16_t pulse_us; /* 最近写入 CCR 的脉宽，us。不表示实际转速。 */
  uint16_t target_pulse_us; /* 下一周期的目标脉宽，us。 */
  bool initialized; /* PWM 启动成功。不表示电调已经就绪。 */
} Snail2305_HandleTypeDef;

/**
 * @brief 校验 TIM1 并以停止脉宽启动一个通道。
 * @param motor 零初始化的持久句柄。每通道只能有一个句柄。
 * @param config 1 MHz、50 Hz 的 TIM1 通道配置。
 * @retval true PWM 已启动。false 配置错误或 HAL 启动失败。
 * @note 仅所属任务调用。失败保留停止 CCR，不修改定时器时基。
 */
bool Snail2305_Init(Snail2305_HandleTypeDef *motor, const Snail2305_ConfigTypeDef *config);
/**
 * @brief 立即写脉宽并重置斜坡历史。
 * @param motor 已启动句柄。
 * @param pulse_us 校准范围内的脉宽，us。
 * @retval true 已写 CCR。false 参数非法，旧值不变。
 * @note 仅所属任务调用。CCR 预装载在下个 PWM 周期生效，最迟 20 ms。
 */
bool Snail2305_SetPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us);
/**
 * @brief 更新停止脉宽并立即输出该值。
 * @param motor 已启动句柄。
 * @param pulse_us 新的停止脉宽，us；必须位于 400~2200 us 且小于最大脉宽。
 * @retval true 已更新配置并写入 CCR。false 参数非法，旧配置不变。
 * @note 仅所属任务调用。修改后会清除 Ramp 历史，适合校准后的安全停机值。
 */
bool Snail2305_SetStopPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us);
/**
 * @brief 设置斜坡目标，不立即跳变输出。
 * @param motor 已启动句柄。
 * @param pulse_us 校准范围内的目标，us。
 * @retval true 已保存目标。false 参数非法，旧目标不变。
 * @note 仅所属任务调用。需要周期调用 Process。
 */
bool Snail2305_SetTargetPulse(Snail2305_HandleTypeDef *motor, uint16_t pulse_us);
/**
 * @brief 将千分比换为校准脉宽目标。
 * @param motor 已启动句柄。
 * @param output_permille 0~1000‰。0 为停止，1000 为最大脉宽。
 * @retval true 已保存目标。false 参数非法。
 * @note 仅所属任务调用。千分比不是转速反馈。
 */
bool Snail2305_SetOutput(Snail2305_HandleTypeDef *motor, uint16_t output_permille);
/**
 * @brief 推进一次脉宽斜坡并写入 CCR。
 * @param motor 已启动句柄。
 * @param dt_ms 本周期时长，ms。必须大于 0。
 * @retval true 已写 CCR。false 句柄或周期非法。
 * @note 仅所属任务调用。不延时，不使用定时器中断。
 */
bool Snail2305_Process(Snail2305_HandleTypeDef *motor, uint32_t dt_ms);
/**
 * @brief 立即保存停止脉宽，绕过斜坡。
 * @param motor 已启动句柄。
 * @retval true 已写停止 CCR。false 句柄未启动。
 * @note 仅所属任务调用。用于通信故障。PWM 信号保持运行。
 */
bool Snail2305_Stop(Snail2305_HandleTypeDef *motor);
/**
 * @brief 查询 PWM 是否启动成功。
 * @param motor 所属任务的句柄。
 * @retval true 已初始化。false 空指针或未启动。
 * @note 仅所属任务调用。不提供电调在线或实际转速判断。
 */
bool Snail2305_IsInitialized(const Snail2305_HandleTypeDef *motor);
#endif /* SNAIL_2305_H。 */
