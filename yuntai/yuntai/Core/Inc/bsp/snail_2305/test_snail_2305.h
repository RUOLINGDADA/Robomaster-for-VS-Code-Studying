/**
 * @file test_snail_2305.h
 * @brief C615 双摩擦轮独立 PWM 台架测试。
 *
 * 测试只启动 TIM1 CH1/CH2，不访问 CAN、DBUS 或 M2006。
 * 将 SNAIL_2305_TEST_ENABLE 显式改为 1 后重新编译。上电前拆除拨弹机构并准备急停。
 * 默认先输出 1000 us 停止脉宽 3000 ms，再以 500 ms 斜坡到 1520 us，持续运行，再斜坡回 1000 us。
 * 测试参数只在本头文件修改；正式供弹参数仍在 task_feed_motor_config.h。
 * CH1/CH2 转向宏用于记录和校验期望方向；C615 的实际反转需用相线或 DJI Assistant 设置。
 */
#ifndef TEST_SNAIL_2305_H
#define TEST_SNAIL_2305_H /* 防止 C615 测试接口重复包含。 */

#include "bsp/snail_2305/snail_2305.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef SNAIL_2305_TEST_ENABLE
#define SNAIL_2305_TEST_ENABLE 0 /* 独立 C615 PWM 测试开关，0/1；与 C610 角度步长测试互斥。 */
#endif
#ifndef SNAIL_2305_TEST_STOP_PULSE_US
#define SNAIL_2305_TEST_STOP_PULSE_US 1000U /* 测试停止脉宽，us；采用整车例程 FRIC_OFF=1000。 */
#endif
#ifndef SNAIL_2305_TEST_CH1_DIRECTION_SIGN
#define SNAIL_2305_TEST_CH1_DIRECTION_SIGN 1 /* 测试 CH1 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
#endif
#ifndef SNAIL_2305_TEST_CH2_DIRECTION_SIGN
#define SNAIL_2305_TEST_CH2_DIRECTION_SIGN (-1) /* 测试 CH2 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
#endif
#ifndef SNAIL_2305_TEST_ACTIVE_PULSE_US
#define SNAIL_2305_TEST_ACTIVE_PULSE_US 1520U /* 测试常规活动脉宽，us；沿用整车例程 FRIC_DOWN=1520。 */
#endif
#ifndef SNAIL_2305_TEST_MAX_PULSE_US
#define SNAIL_2305_TEST_MAX_PULSE_US 1550U /* 测试活动上限，us；沿用整车例程 FRIC_UP=1550。 */
#endif
#ifndef SNAIL_2305_TEST_RAMP_TIME_MS
#define SNAIL_2305_TEST_RAMP_TIME_MS 500U /* 到活动值和返回停止值的时间，ms；过短会增加冲击。 */
#endif
#ifndef SNAIL_2305_TEST_STARTUP_STOP_TIME_MS
#define SNAIL_2305_TEST_STARTUP_STOP_TIME_MS 3000U /* 启动停止等待，ms；电调未识别停止脉宽时增加。 */
#endif
#ifndef SNAIL_2305_TEST_RUN_TIME_MS
#define SNAIL_2305_TEST_RUN_TIME_MS 0U /* 活动保持时间，ms；0 表示持续运行，台架风险更高。 */
#endif
#ifndef SNAIL_2305_TEST_LOG_PERIOD_MS
#define SNAIL_2305_TEST_LOG_PERIOD_MS 500U /* 测试日志周期，ms；串口繁忙时增大。 */
#endif

#if SNAIL_2305_TEST_ENABLE != 0 && SNAIL_2305_TEST_ENABLE != 1
#error "SNAIL_2305_TEST_ENABLE must be 0 or 1"
#endif
_Static_assert((SNAIL_2305_TEST_CH1_DIRECTION_SIGN == 1 ||
                SNAIL_2305_TEST_CH1_DIRECTION_SIGN == -1) &&
               (SNAIL_2305_TEST_CH2_DIRECTION_SIGN == 1 ||
                SNAIL_2305_TEST_CH2_DIRECTION_SIGN == -1),
               "Snail 2305 test directions must be +1 or -1");

_Static_assert(SNAIL_2305_TEST_STOP_PULSE_US >= SNAIL_2305_MIN_PROTOCOL_PULSE_US &&
               SNAIL_2305_TEST_STOP_PULSE_US < SNAIL_2305_TEST_ACTIVE_PULSE_US &&
               SNAIL_2305_TEST_ACTIVE_PULSE_US <= SNAIL_2305_TEST_MAX_PULSE_US &&
               SNAIL_2305_TEST_MAX_PULSE_US <= SNAIL_2305_MAX_PROTOCOL_PULSE_US &&
               SNAIL_2305_TEST_RAMP_TIME_MS > 0U &&
               SNAIL_2305_TEST_RAMP_TIME_MS <= INT32_MAX &&
               SNAIL_2305_TEST_STARTUP_STOP_TIME_MS <= INT32_MAX &&
               SNAIL_2305_TEST_LOG_PERIOD_MS > 0U &&
               SNAIL_2305_TEST_LOG_PERIOD_MS <= INT32_MAX,
               "invalid Snail 2305 test configuration");

typedef struct {
  Snail2305_HandleTypeDef left; /* PE9/TIM1_CH1 测试句柄；测试任务独占。 */
  Snail2305_HandleTypeDef right; /* PE11/TIM1_CH2 测试句柄；测试任务独占。 */
  uint32_t start_ms; /* 测试开始时刻，HAL ms；0 ms 也有效。 */
  uint32_t last_log_ms; /* 最近成功日志时刻，HAL ms。 */
  bool started; /* 已记录测试起点；不使用时间戳 0 判断是否初始化。 */
  bool initialized; /* 两个 PWM 均已启动。 */
  bool log_started; /* 至少提交过一条测试日志。 */
  bool finished; /* 已进入停止 Ramp，避免重复计算起点。 */
} Snail2305_TestStateTypeDef;

/**
 * @brief 初始化两个 C615 测试 PWM。
 * @param test 任务独占的持久测试状态。
 * @param htim 已执行 MX_TIM1_Init 的 TIM1 句柄。
 * @retval true 两个通道已启动并输出停止脉宽；false 参数或 HAL 初始化失败。
 * @note 仅供弹任务上下文调用；不检查 C615 电调在线状态。
 */
bool Snail2305_Test_Init(Snail2305_TestStateTypeDef *test,
                         TIM_HandleTypeDef *htim);

/**
 * @brief 推进一步 C615 测试斜坡。
 * @param test 已初始化的测试状态。
 * @param now_ms 当前 HAL ms。
 * @param dt_ms 本周期时长，ms；必须大于 0。
 * @retval None。初始化失败或完成后保持停止脉宽。
 * @note 仅供弹任务上下文调用；不访问 CAN，不等待 PWM 周期。
 */
void Snail2305_Test_RunCycle(Snail2305_TestStateTypeDef *test,
                             uint32_t now_ms, uint32_t dt_ms);

#endif /* TEST_SNAIL_2305_H */
