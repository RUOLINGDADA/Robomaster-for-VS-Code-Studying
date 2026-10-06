/**
 * @file log_config.h
 * @brief 所有 USART1 日志的编译期开关和图表前缀。
 *
 * 只看一个电机时关闭其它电机分类。只看图表时关闭 TASK、TEST 和 DBUS_TEXT。
 * 日志过多时关闭分类，或增大对应任务的日志周期。改宏后重新编译并烧录。
 * 本文件不配置外设。USART3 只接收 DBUS。
 */
#ifndef APP_LOG_CONFIG_H
#define APP_LOG_CONFIG_H /* 防止日志配置重复包含。 */

#ifndef LOG_GLOBAL_ENABLE
#define LOG_GLOBAL_ENABLE 1U /* 日志总开关，0/1；关闭全部格式化和发送。 */
#endif
#ifndef LOG_USART1_ENABLE
#define LOG_USART1_ENABLE 1U /* USART1 输出开关，0/1；不改变 DBUS 接收。 */
#endif
#ifndef LOG_YAW_ENABLE
#define LOG_YAW_ENABLE 0U /* Yaw 日志开关，0/1；关闭本轴正式、标定和测试日志。 */
#endif
#ifndef LOG_PITCH_ENABLE
#define LOG_PITCH_ENABLE 0U /* Pitch 日志开关，0/1；关闭本轴正式、标定和测试日志。 */
#endif
#ifndef LOG_FEED_MOTOR_ENABLE
#define LOG_FEED_MOTOR_ENABLE 1U /* 供弹日志开关，0/1；关闭初始化、正式和 C610 测试日志。 */
#endif
#ifndef LOG_TASK_ENABLE
#define LOG_TASK_ENABLE 0U /* 正式任务日志总开关，0/1；与各电机开关同时开启才输出。 */
#endif
#ifndef LOG_TEST_ENABLE
#define LOG_TEST_ENABLE 1U /* 硬件测试日志总开关，0/1；与各电机开关同时开启才输出。 */
#endif
#ifndef LOG_DBUS_TEXT_ENABLE
#define LOG_DBUS_TEXT_ENABLE 0U /* DBUS 文本开关，0/1；输出原始数据和接收状态。 */
#endif
#ifndef LOG_DBUS_CHART_ENABLE
#define LOG_DBUS_CHART_ENABLE 0U /* DBUS 图表开关，0/1；输出 10 通道 ch: 帧。 */
#endif
#ifndef LOG_CHART_PREFIX
#define LOG_CHART_PREFIX "ch:" /* 图表前缀，1~16 字节；与上位机一致，不含逗号、换行或 %。 */
#endif

#define LOG_SWITCH_VALID(value) ((value) == 0U || (value) == 1U) /* 开关只接受 0/1，拒绝负数和时长。 */
_Static_assert(LOG_SWITCH_VALID(LOG_GLOBAL_ENABLE) && LOG_SWITCH_VALID(LOG_USART1_ENABLE) &&
               LOG_SWITCH_VALID(LOG_YAW_ENABLE) && LOG_SWITCH_VALID(LOG_PITCH_ENABLE) &&
               LOG_SWITCH_VALID(LOG_FEED_MOTOR_ENABLE) && LOG_SWITCH_VALID(LOG_TASK_ENABLE) &&
               LOG_SWITCH_VALID(LOG_TEST_ENABLE) && LOG_SWITCH_VALID(LOG_DBUS_TEXT_ENABLE) &&
               LOG_SWITCH_VALID(LOG_DBUS_CHART_ENABLE), "log switches must be 0 or 1");

#endif /* APP_LOG_CONFIG_H */
