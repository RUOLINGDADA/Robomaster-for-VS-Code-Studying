/**
 * @file task_feed_motor_config.h
 * @brief 供弹任务、C615 和 C610 上板测试参数。
 *
 * 供弹不动时先检查 CAN ID、反馈和方向。过冲时降低位置 P。
 * 到位误判时增加窗口或确认时间；这会降低发射频率。
 * 摩擦轮温升高时降低活动脉宽。启动冲击大时增加 Ramp 时间。
 * 自循环仅用于上板调试。修改后重新编译并烧录。
 */
#ifndef TASK_FEED_MOTOR_CONFIG_H
#define TASK_FEED_MOTOR_CONFIG_H /* 防止重复包含。 */

#include <stdint.h>

/* C615 模式只改下面这一行：0=正式供弹，1=PWM 行程校准，2=转向切换校准。
 * SNAIL_2305_TEST_ENABLE 是另一套独立的摩擦轮台架测试开关，不用于选择正式/校准模式。 */
#define FEED_MOTOR_SNAIL_MODE_FORMAL 0U /* 正式供弹模式；DBUS 左键控制 C615 与 M2006。 */
#define FEED_MOTOR_SNAIL_MODE_PWM_CALIBRATION 1U /* C615 PWM 行程校准模式；只输出校准 PWM。 */
#define FEED_MOTOR_SNAIL_MODE_DIRECTION_CALIBRATION 2U /* C615 转向切换模式；只输出校准 PWM。 */
#ifndef FEED_MOTOR_SNAIL_MODE
#define FEED_MOTOR_SNAIL_MODE FEED_MOTOR_SNAIL_MODE_FORMAL /* 唯一模式选择：0 正式，1 行程校准，2 转向校准。 */
#endif
_Static_assert(FEED_MOTOR_SNAIL_MODE >= FEED_MOTOR_SNAIL_MODE_FORMAL &&
               FEED_MOTOR_SNAIL_MODE <= FEED_MOTOR_SNAIL_MODE_DIRECTION_CALIBRATION,
               "FEED_MOTOR_SNAIL_MODE must be 0, 1, or 2");
#define FEED_MOTOR_TASK_PERIOD_MS 2U /* 控制周期，ms；修改后检查调度负载。 */
#define FEED_MOTOR_INIT_RETRY_MS 100U /* 初始化重试间隔，ms；避免故障忙等。 */
#define FEED_MOTOR_COMMAND_TIMEOUT_MS 100U /* DBUS 命令期限，HAL ms；过期立即停机。 */
#define FEED_MOTOR_ID 1U /* CAN1 C610 ID1；反馈标准帧 0x201，控制标准帧 0x200。 */
#define FEED_MOTOR_STEP_COUNTS 409 /* 每发步长，电机轴 count；拨弹量错误时按机械传动标定。 */
#define FEED_MOTOR_WINDOW_COUNTS 65 /* 到位误差，电机轴 count；增大会提前结束一发。 */
#define FEED_MOTOR_READY_SPEED_RPM 10 /* 到位转速阈值，电机轴 rpm；提高会增加惯性误判。 */
#define FEED_MOTOR_SETTLE_MS 20U /* 到位条件连续确认时长，ms；减少会增加噪声误判。 */
#define FEED_MOTOR_SHOT_INTERVAL_MS 100U /* 每发完成后的最小等待，ms；减小会提高供弹频率。 */
#define FEED_MOTOR_SPINUP_MS 300U /* 左键按下后的最小预旋时长，ms；还需两轮到达目标脉宽。 */
#define FEED_MOTOR_POSITION_KP 2.0f /* 位置 P，C610 raw/count；响应慢时增加，过冲时降低。 */
#define FEED_MOTOR_MAX_CURRENT_RAW 700 /* 台架限流，C610 raw，范围 (0,10000]；提高可增力矩，也增加温升与卡弹损伤。 */
#define FEED_MOTOR_FEEDBACK_SIGN -1 /* 角度/速度反馈到逻辑坐标的符号，±1；待实测。 */
#define FEED_MOTOR_CURRENT_SIGN -1 /* 逻辑电流到协议电流的符号，±1；与反馈符号分开调。 */
#define FEED_MOTOR_LOG_PERIOD_MS 500U /* 诊断间隔，ms；串口繁忙时重试。 */
#ifndef FEED_MOTOR_SNAIL_STOP_PULSE_US
#define FEED_MOTOR_SNAIL_STOP_PULSE_US 1000U /* C615 停止脉宽，us；停转仍异常时先重新校准。 */
#endif
#ifndef FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN
#define FEED_MOTOR_SNAIL_CH1_DIRECTION_SIGN 1 /* 正式 CH1 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
#endif
#ifndef FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN
#define FEED_MOTOR_SNAIL_CH2_DIRECTION_SIGN (-1) /* 正式 CH2 期望转向，+1/-1；实际方向由 C615 相线或 Assistant 设置。 */
#endif
#ifndef FEED_MOTOR_SNAIL_MAX_PULSE_US
#define FEED_MOTOR_SNAIL_MAX_PULSE_US 1550U /* C615 正式运行上限，us；沿用整车例程 FRIC_UP=1550。 */
#endif
#ifndef FEED_MOTOR_SNAIL_ACTIVE_PULSE_US
#define FEED_MOTOR_SNAIL_ACTIVE_PULSE_US 1520U /* C615 常规发射脉宽，us；沿用整车例程 FRIC_DOWN=1520。 */
#endif
#ifndef FEED_MOTOR_SNAIL_RAMP_TIME_MS
#define FEED_MOTOR_SNAIL_RAMP_TIME_MS 300U /* C615 加减速时间，ms；过短会增加启动冲击。 */
#endif
#ifndef FEED_MOTOR_TEST_ENABLE
#define FEED_MOTOR_TEST_ENABLE 0 /* C610 自循环开关，0/1；上板调试时确认可急停。 */
#endif
#ifndef FEED_MOTOR_TEST_UP_TIME_MS
#define FEED_MOTOR_TEST_UP_TIME_MS 500U /* 自循环正向时间，ms；动作过短时增加。 */
#endif
#ifndef FEED_MOTOR_TEST_STOP_TIME_MS
#define FEED_MOTOR_TEST_STOP_TIME_MS 500U /* 自循环换向停顿，ms；换向冲击时增加。 */
#endif
#ifndef FEED_MOTOR_TEST_DOWN_TIME_MS
#define FEED_MOTOR_TEST_DOWN_TIME_MS 500U /* 自循环反向时间，ms；动作过短时增加。 */
#endif
#ifndef FEED_MOTOR_TEST_UP_CURRENT_RAW
#define FEED_MOTOR_TEST_UP_CURRENT_RAW 700 /* 自循环正向电流，C610 原始值；力不足时增加并检查温升。 */
#endif
#ifndef FEED_MOTOR_TEST_DOWN_CURRENT_RAW
#define FEED_MOTOR_TEST_DOWN_CURRENT_RAW (-500) /* 自循环反向电流，C610 原始值；方向错误时先改符号。 */
#endif
#ifndef FEED_MOTOR_TEST_LOG_PERIOD_MS
#define FEED_MOTOR_TEST_LOG_PERIOD_MS 500U /* 自循环日志间隔，ms；串口刷屏时增加。 */
#endif
#if FEED_MOTOR_TEST_ENABLE != 0 && FEED_MOTOR_TEST_ENABLE != 1
#error "FEED_MOTOR_TEST_ENABLE must be 0 or 1"
#endif

_Static_assert(FEED_MOTOR_TEST_UP_TIME_MS > 0U && FEED_MOTOR_TEST_UP_TIME_MS <= INT32_MAX &&
               FEED_MOTOR_TEST_STOP_TIME_MS > 0U && FEED_MOTOR_TEST_STOP_TIME_MS <= INT32_MAX &&
               FEED_MOTOR_TEST_DOWN_TIME_MS > 0U && FEED_MOTOR_TEST_DOWN_TIME_MS <= INT32_MAX &&
               FEED_MOTOR_TEST_LOG_PERIOD_MS > 0U && FEED_MOTOR_TEST_LOG_PERIOD_MS <= INT32_MAX,
               "invalid C610 test durations");
#endif /* TASK_FEED_MOTOR_CONFIG_H */
