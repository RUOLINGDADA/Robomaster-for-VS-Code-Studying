/**
 * @file task_feed_motor_config.h
 * @brief 供弹任务、C615 和 C610 上板测试参数。
 *
 * 供弹按连续角度固定步长运行。左键短按完成一个 `FEED_MOTOR_STEP_COUNTS`；
 * 长按达到阈值后按“步长→停顿→步长”重复；连发过快时增大 STEP_PAUSE_MS。
 * Shift 只控制 C615 摩擦轮；松开直接写停止脉宽。
 * 供弹方向由步长符号和电流符号共同决定；反馈符号要与逻辑坐标匹配。
 * 摩擦轮温升高时降低活动脉宽；启动冲击大时增加启动 Ramp 时间。
 * C610 角度步长测试仅用于上板标定。测试参数与正式供弹参数分开；测试完成后
 * 必须把实测值人工写入正式参数，并将测试开关恢复为 0。修改后重新编译并烧录。
 */
#ifndef TASK_FEED_MOTOR_CONFIG_H
#define TASK_FEED_MOTOR_CONFIG_H /* 防止重复包含。 */

#include <stdint.h>

#define FEED_MOTOR_TASK_PERIOD_MS 2U /* 控制周期，ms；修改后检查调度负载。 */
#define FEED_MOTOR_INIT_RETRY_MS 100U /* 初始化重试间隔，ms；避免故障忙等。 */
#define FEED_MOTOR_COMMAND_TIMEOUT_MS 100U /* DBUS 命令期限，HAL ms；过期立即停机。 */
#define FEED_MOTOR_ID 1U /* CAN2 C610 ID1；反馈标准帧 0x201，控制标准帧 0x200。 */
#ifndef FEED_MOTOR_STEP_COUNTS
#define FEED_MOTOR_STEP_COUNTS (4000LL) /* 每次供弹的电机转子连续逻辑 count，非零；用户指定试用值，符号决定目标和逻辑电流方向。 */
#endif
#define FEED_MOTOR_CONTINUOUS_PRESS_MS 400U /* 长按阈值，ms；短按完成当前步后停止，长按才建立下一步。 */
#ifndef FEED_MOTOR_STEP_PAUSE_MS
#define FEED_MOTOR_STEP_PAUSE_MS 100U /* 长按每步完成后的零电流停顿，ms，范围 (0,INT32_MAX]；增大会降低重复频率。 */
#endif
#define FEED_MOTOR_FEED_CURRENT_RAW 5000 /* M2006 连续供弹电流，C610 raw；增加会增大力矩和温升。 */
#define FEED_MOTOR_MAX_CURRENT_RAW 5000 /* C610 电流安全上限，raw，范围 (0,10000]；用于钳位供弹电流。 */
#define FEED_MOTOR_FEEDBACK_SIGN 1 /* 角度/速度反馈到逻辑坐标的符号，±1；待实测。 */
#define FEED_MOTOR_CURRENT_SIGN 1 /* 逻辑电流到协议电流的符号，±1；最终电流=步长符号×本符号×电流幅值。 */
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
#define FEED_MOTOR_SNAIL_ACTIVE_PULSE_US 1400U /* C615 正式活动脉宽，us，范围 (STOP,MAX]；保留当前调参值，需实测带载起转和温升。 */
#endif
#ifndef FEED_MOTOR_SNAIL_RAMP_TIME_MS
#define FEED_MOTOR_SNAIL_RAMP_TIME_MS 10U /* C615 启动 Ramp 时间，ms，须为正；松键停止不等待 Ramp。 */
#endif
/* C610 手动角度测试参数：只在 FEED_MOTOR_ANGLE_STEP_TEST_ENABLE=1 的测试构建中消费。
 * 测试只读反馈并输出零电流；操作者手动转动拨弹机构，根据日志确认反馈方向，
 * 再把本开关恢复为 0；这些测试宏不会被正式运行时读取。 */
#ifndef FEED_MOTOR_ANGLE_STEP_TEST_ENABLE
#define FEED_MOTOR_ANGLE_STEP_TEST_ENABLE 0 /* C610 手动角度步长测试开关，0/1；开启后只读测量，完成后恢复 0。 */
#endif
#ifndef FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN
#define FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN (-1) /* 测试反馈方向，±1；与正式 FEED_MOTOR_FEEDBACK_SIGN 独立。 */
#endif
#ifndef FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS
#define FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS 100U /* 当前角度日志间隔，ms；成功提交后再计时。 */
#endif

#if FEED_MOTOR_ANGLE_STEP_TEST_ENABLE != 0 && FEED_MOTOR_ANGLE_STEP_TEST_ENABLE != 1
#error "FEED_MOTOR_ANGLE_STEP_TEST_ENABLE must be 0 or 1"
#endif
_Static_assert((FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN == 1 ||
                FEED_MOTOR_ANGLE_STEP_TEST_FEEDBACK_SIGN == -1) &&
               FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS > 0U &&
               FEED_MOTOR_ANGLE_STEP_TEST_LOG_PERIOD_MS <= INT32_MAX,
               "invalid C610 angle step test configuration");
_Static_assert(FEED_MOTOR_STEP_COUNTS != 0 &&
               FEED_MOTOR_CONTINUOUS_PRESS_MS > 0U &&
               FEED_MOTOR_CONTINUOUS_PRESS_MS <= INT32_MAX &&
               FEED_MOTOR_STEP_PAUSE_MS > 0U &&
               FEED_MOTOR_STEP_PAUSE_MS <= INT32_MAX &&
               FEED_MOTOR_FEED_CURRENT_RAW > 0 &&
               FEED_MOTOR_FEED_CURRENT_RAW <= FEED_MOTOR_MAX_CURRENT_RAW &&
               FEED_MOTOR_MAX_CURRENT_RAW <= 10000 &&
               (FEED_MOTOR_FEEDBACK_SIGN == 1 || FEED_MOTOR_FEEDBACK_SIGN == -1) &&
               (FEED_MOTOR_CURRENT_SIGN == 1 || FEED_MOTOR_CURRENT_SIGN == -1),
               "invalid feed step/current/sign configuration");
#endif /* TASK_FEED_MOTOR_CONFIG_H */
