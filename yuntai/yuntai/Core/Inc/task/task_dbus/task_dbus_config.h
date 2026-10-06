/**
 * @file task_dbus_config.h
 * @brief DBUS 任务输入和遥测参数。
 *
 * 摇杆方向错误时调整 INPUT_SIGN。摇杆回中漂移时调整 DEADBAND_RAW。
 * 鼠标响应过快时减小对应轴的 GAIN。停手后保持过久时减小 HOLD_MS。
 * 回中过急或仍摆动时增大 DECAY_MS。输出上限只用于安全限制。
 */
#ifndef TASK_DBUS_CONFIG_H
#define TASK_DBUS_CONFIG_H /* 防止配置重复包含。 */

#include <stdint.h>

#define DBUS_TASK_PERIOD_MS 2U /* 任务周期，ms；修改后检查调度负载。 */
#define DBUS_TASK_LOG_PERIOD_MS 100U /* DBUS 遥测周期，ms；增大会降低日志带宽。 */
#define DBUS_TASK_INIT_RETRY_MS 100U /* 接收器初始化重试周期，ms；过小会增加故障时 CPU 占用。 */
#define DBUS_STICK_DEADBAND_RAW 10 /* 摇杆回中死区，原始计数；过大会吞掉小动作。 */
#define DBUS_YAW_CHANNEL 0U /* Yaw 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
#define DBUS_PITCH_CHANNEL 1U /* Pitch 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
#define DBUS_YAW_INPUT_SIGN 1 /* Yaw 摇杆方向，+1/-1；方向反时只改此项。 */
#define DBUS_PITCH_INPUT_SIGN 1 /* Pitch 摇杆方向，+1/-1；方向反时只改此项。 */

#ifndef DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT
#define DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 0.5f /* Yaw 增益，‰/count，有限非负数；过快时减小，可用 0.05f。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS
#define DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS 70U /* Yaw 保持时间，ms，可为 0；运动拖尾过长时减小。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS
#define DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS 70U /* Yaw 回中时间，ms，>0；回中冲击或摆动时增大。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_SIGN
#define DBUS_YAW_MOUSE_VIRTUAL_SIGN (-1) /* Yaw 鼠标方向，+1/-1；方向反时只改此项。 */
#endif

#ifndef DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT
#define DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 0.9f /* Pitch 增益，‰/count，有限非负数；过快时减小，可用 0.05f。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS
#define DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS 70U /* Pitch 保持时间，ms，可为 0；运动拖尾过长时减小。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS
#define DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS 70U /* Pitch 回中时间，ms，>0；回中冲击或摆动时增大。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_SIGN
#define DBUS_PITCH_MOUSE_VIRTUAL_SIGN 1 /* Pitch 鼠标方向，+1/-1；方向反时只改此项。 */
#endif

#ifndef DBUS_MOUSE_VIRTUAL_ENABLE
#define DBUS_MOUSE_VIRTUAL_ENABLE 1U /* 虚拟鼠标总开关，0/1；异常时可关闭鼠标控制。 */
#endif
#ifndef DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE
#define DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE 660 /* 鼠标上限，1~1000‰；降低会减少最大响应。 */
#endif

_Static_assert(DBUS_MOUSE_VIRTUAL_ENABLE == 0U || DBUS_MOUSE_VIRTUAL_ENABLE == 1U,
               "virtual mouse switch must be 0 or 1");
_Static_assert(DBUS_YAW_MOUSE_VIRTUAL_SIGN == 1 || DBUS_YAW_MOUSE_VIRTUAL_SIGN == -1,
               "yaw mouse sign must be +1 or -1");
_Static_assert(DBUS_PITCH_MOUSE_VIRTUAL_SIGN == 1 || DBUS_PITCH_MOUSE_VIRTUAL_SIGN == -1,
               "pitch mouse sign must be +1 or -1");
_Static_assert(DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS > 0U &&
               (uint64_t)DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS <= INT32_MAX &&
               (uint64_t)DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS <= INT32_MAX &&
               (uint64_t)DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS + DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS <= INT32_MAX,
               "yaw mouse durations are invalid");
_Static_assert(DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS > 0U &&
               (uint64_t)DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS <= INT32_MAX &&
               (uint64_t)DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS <= INT32_MAX &&
               (uint64_t)DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS + DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS <= INT32_MAX,
               "pitch mouse durations are invalid");
_Static_assert(DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE > 0 &&
               DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE <= 1000,
               "mouse output limit must be 1..1000 permille");

#endif /* TASK_DBUS_CONFIG_H */
