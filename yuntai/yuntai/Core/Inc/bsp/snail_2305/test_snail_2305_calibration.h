/**
 * @file test_snail_2305_calibration.h
 * @brief C615 PWM 行程和转向切换校准测试。
 *
 * 测试只访问 TIM1 CH1/CH2，不访问 CAN、DBUS 或 M2006。启动后立即输出最大
 * 校准脉宽；保持指定毫秒数后切换到最小脉宽并保持。电调的 BB、BBB、B 声由
 * 电调发出，MCU 无法读取，因此操作人员必须在对应提示音窗口启动测试并确认结果。
 * 函数只推进一个非阻塞周期，不调用 HAL_Delay；上电前拆除拨弹机构并准备急停。
 */
#ifndef TEST_SNAIL_2305_CALIBRATION_H
#define TEST_SNAIL_2305_CALIBRATION_H /* 防止 C615 校准接口重复包含。 */

#include "bsp/snail_2305/snail_2305.h"
#include "task/task_feed_motor/task_feed_motor_config.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef SNAIL_2305_CALIBRATION_MIN_PULSE_US
#define SNAIL_2305_CALIBRATION_MIN_PULSE_US 1000U /* 校准最小脉宽，us；应与 C615 当前最小行程一致。 */
#endif
#ifndef SNAIL_2305_CALIBRATION_MAX_PULSE_US
#define SNAIL_2305_CALIBRATION_MAX_PULSE_US 2200U /* 校准最大脉宽，us；应与 C615 当前最大行程一致。 */
#endif
#ifndef SNAIL_2305_CALIBRATION_MAX_HOLD_MS
#if FEED_MOTOR_SNAIL_MODE == FEED_MOTOR_SNAIL_MODE_DIRECTION_CALIBRATION
#define SNAIL_2305_CALIBRATION_MAX_HOLD_MS 4000U /* 转向校准最大脉宽保持时间，ms；覆盖 BB 后的 BBB 提示音窗口，需人工听音确认。 */
#else
#define SNAIL_2305_CALIBRATION_MAX_HOLD_MS 2000U /* 行程校准最大脉宽保持时间，ms；对应 BB 提示音窗口，需人工听音确认。 */
#endif
#endif
#ifndef SNAIL_2305_CALIBRATION_MIN_HOLD_MS
#define SNAIL_2305_CALIBRATION_MIN_HOLD_MS 5000U /* 最小脉宽保持时间，ms；覆盖电调确认 B 声的时间。 */
#endif
#ifndef SNAIL_2305_CALIBRATION_CH1_DIRECTION_SIGN
#define SNAIL_2305_CALIBRATION_CH1_DIRECTION_SIGN 1 /* 校准 CH1 期望转向，+1/-1；实际方向由相线或 Assistant 设置。 */
#endif
#ifndef SNAIL_2305_CALIBRATION_CH2_DIRECTION_SIGN
#define SNAIL_2305_CALIBRATION_CH2_DIRECTION_SIGN (-1) /* 校准 CH2 期望转向，+1/-1；实际方向由相线或 Assistant 设置。 */
#endif

_Static_assert(FEED_MOTOR_SNAIL_MODE <= FEED_MOTOR_SNAIL_MODE_DIRECTION_CALIBRATION,
               "invalid Snail 2305 calibration mode");
_Static_assert(SNAIL_2305_CALIBRATION_MIN_PULSE_US >= SNAIL_2305_MIN_PROTOCOL_PULSE_US &&
               SNAIL_2305_CALIBRATION_MIN_PULSE_US < SNAIL_2305_CALIBRATION_MAX_PULSE_US &&
               SNAIL_2305_CALIBRATION_MAX_PULSE_US <= SNAIL_2305_MAX_PROTOCOL_PULSE_US &&
               SNAIL_2305_CALIBRATION_MAX_HOLD_MS > 0U &&
               SNAIL_2305_CALIBRATION_MAX_HOLD_MS <= INT32_MAX &&
               SNAIL_2305_CALIBRATION_MIN_HOLD_MS > 0U &&
               SNAIL_2305_CALIBRATION_MIN_HOLD_MS <= INT32_MAX,
               "invalid Snail 2305 calibration timing or pulse");
_Static_assert((SNAIL_2305_CALIBRATION_CH1_DIRECTION_SIGN == 1 ||
                SNAIL_2305_CALIBRATION_CH1_DIRECTION_SIGN == -1) &&
               (SNAIL_2305_CALIBRATION_CH2_DIRECTION_SIGN == 1 ||
                SNAIL_2305_CALIBRATION_CH2_DIRECTION_SIGN == -1),
               "Snail 2305 calibration directions must be +1 or -1");

typedef enum {
  SNAIL_2305_CALIBRATION_PHASE_MAX = 0,
  SNAIL_2305_CALIBRATION_PHASE_MIN,
  SNAIL_2305_CALIBRATION_PHASE_DONE
} Snail2305_CalibrationPhaseTypeDef;

typedef struct {
  Snail2305_HandleTypeDef ch1; /* PE9/TIM1_CH1 校准句柄；校准任务独占。 */
  Snail2305_HandleTypeDef ch2; /* PE11/TIM1_CH2 校准句柄；校准任务独占。 */
  uint32_t phase_start_ms; /* 当前阶段开始时刻，HAL ms；0 ms 也有效。 */
  bool started; /* 已记录阶段起点；不能用时间戳 0 判断。 */
  bool initialized; /* 两个 PWM 均已启动并输出最大脉宽。 */
  bool prompt_max_logged; /* 最大阶段提示是否已成功提交。 */
  bool prompt_min_logged; /* 最小阶段提示是否已成功提交。 */
  bool prompt_done_logged; /* 完成提示是否已成功提交。 */
  Snail2305_CalibrationPhaseTypeDef phase; /* 最大、最小或完成阶段。 */
} Snail2305_CalibrationStateTypeDef;

/**
 * @brief 初始化 C615 校准并立即输出最大脉宽。
 * @param state 任务独占且地址固定的校准状态。
 * @param htim 已执行 MX_TIM1_Init 的 TIM1 句柄。
 * @retval true 两个通道已启动并输出最大脉宽；false 参数或 HAL 失败。
 * @note 仅任务上下文调用；初始化不等待电调鸣音。
 */
bool Snail2305_Calibration_Init(Snail2305_CalibrationStateTypeDef *state,
                                TIM_HandleTypeDef *htim);

/**
 * @brief 推进一步 C615 校准状态机。
 * @param state 已初始化的校准状态。
 * @param now_ms 当前 HAL 时间，ms。
 * @param dt_ms 本周期时长，ms；必须大于 0。
 * @retval None；校准完成后继续保持最小脉宽。
 * @note 仅任务上下文调用，不阻塞、不访问 CAN。
 */
void Snail2305_Calibration_RunCycle(Snail2305_CalibrationStateTypeDef *state,
                                    uint32_t now_ms, uint32_t dt_ms);

#endif /* TEST_SNAIL_2305_CALIBRATION_H */
