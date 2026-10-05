/**
 * @file task_dbus_config.h
 * @brief 遥控输入映射与时序配置（输入方向不等于电机电流极性，不能混着改）。
 *
 * 摇杆方向反了调 *_INPUT_SIGN；回中漂移调 DEADBAND_RAW；移动快慢调各轴
 * MAX_COMMAND_SPEED_RPM；跟踪迟缓先检查速度/电流限幅，再调各轴控制参数。
 */
#ifndef TASK_DBUS_CONFIG_H
#define TASK_DBUS_CONFIG_H /* 防止 DBUS 配置重复包含（保持各调用点使用同一组参数）。 */
#define DBUS_TASK_PERIOD_MS 2U /* 任务周期，单位 ms，使用 pdMS_TO_TICKS 换算（不能直接把毫秒当 Tick）。 */
#define DBUS_TASK_LOG_PERIOD_MS 500U /* 普通诊断间隔，单位 HAL ms（UART 忙时下次重试，不阻塞）。 */
#define DBUS_TASK_INIT_RETRY_MS 100U /* 初始化失败重试间隔，单位 ms（不因接收器故障忙等）。 */
#define DBUS_STICK_DEADBAND_RAW 10 /* 回中死区，单位原始计数，范围 0~659（过大会使小动作没反应）。 */
#define DBUS_YAW_CHANNEL 0U /* 官方右摇杆左右 CH0（不采用官方云台示例的左摇杆 CH2）。 */
#define DBUS_PITCH_CHANNEL 1U /* 官方右摇杆上下 CH1（两轴从同一 DBUS 快照取值）。 */
#define DBUS_YAW_INPUT_SIGN 1 /* 遥控左右到连续角度方向，±1（方向反了改这里，不改 PID 符号）。 */
#define DBUS_PITCH_INPUT_SIGN 1 /* 遥控上下到连续角度方向，±1（实测后让上推对应抬头）。 */
#endif /* TASK_DBUS_CONFIG_H */
