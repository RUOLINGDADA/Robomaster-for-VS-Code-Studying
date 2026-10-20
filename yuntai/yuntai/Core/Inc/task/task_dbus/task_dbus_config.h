/**
 * @file task_dbus_config.h
 * @brief DBUS 任务输入和遥测参数。
 *
 * 摇杆方向错误时调整 INPUT_SIGN。摇杆回中漂移时调整 DEADBAND_RAW。
 * 鼠标响应过快时减小对应轴的 GAIN。REFERENCE_PERIOD_MS 规定增益对应的接收周期。
 * HOLD_MS/DECAY_MS 仅控制缺帧时的保持和回中，零位移新帧直接停止。输出上限限制鼠标速度。
 * 输入采集的缓存容量和静止心跳在 TRACE 参数组设置；日志开关在 log_config.h。
 */
#ifndef TASK_DBUS_CONFIG_H
#define TASK_DBUS_CONFIG_H /* 防止配置重复包含。 */

#include <stdint.h>

#define DBUS_TASK_PERIOD_MS 2U /* 任务周期，ms；修改后检查调度负载。 */
#define DBUS_TASK_LOG_PERIOD_MS 100U /* DBUS 遥测周期，ms；增大会降低日志带宽。 */
#define DBUS_TRACE_CAPACITY 128U /* 任务独占诊断环的记录数，16~512；满时丢新记录并计数。 */
#define DBUS_TRACE_BATCH_BYTES 704U /* 单次文本批次容量，256~767 字节；小于共用 DMA 缓冲。 */
#define DBUS_TRACE_HEARTBEAT_MS 100U /* 命令不变时的记录间隔，HAL ms，>0；变化仍逐周期记录。 */
#define DBUS_TASK_INIT_RETRY_MS 100U /* 接收器初始化重试周期，ms；过小会增加故障时 CPU 占用。 */
#define DBUS_STICK_DEADBAND_RAW 10 /* 摇杆回中死区，原始计数；过大会吞掉小动作。 */
#define DBUS_YAW_CHANNEL 0U /* Yaw 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
#define DBUS_PITCH_CHANNEL 1U /* Pitch 摇杆通道，0~3；修改前核对 DBUS 协议映射。 */
#define DBUS_YAW_INPUT_SIGN 1 /* Yaw 摇杆方向，+1/-1；方向反时只改此项。 */
#define DBUS_PITCH_INPUT_SIGN 1 /* Pitch 摇杆方向，+1/-1；方向反时只改此项。 */

#ifndef DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS
#define DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS 14U /* 鼠标测速参考周期，ms，1~INT32_MAX；增大会同比提高速度。 */
#endif

#ifndef DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT
#define DBUS_YAW_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 0.5f /* Yaw 参考周期增益，‰/count，有限非负数；增大会提高灵敏度。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS
#define DBUS_YAW_MOUSE_VIRTUAL_HOLD_MS 14U /* Yaw 缺帧保持，ms，可为 0；过小会在正常帧间提前降速。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS
#define DBUS_YAW_MOUSE_VIRTUAL_DECAY_MS 14U /* Yaw 缺帧回中，ms，>0；增大会延长断续输入的拖尾。 */
#endif
#ifndef DBUS_YAW_MOUSE_VIRTUAL_SIGN
#define DBUS_YAW_MOUSE_VIRTUAL_SIGN (-1) /* Yaw 鼠标方向，+1/-1；方向反时只改此项。 */
#endif

#ifndef DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT
#define DBUS_PITCH_MOUSE_VIRTUAL_GAIN_PERMILLE_PER_COUNT 0.5f /* Pitch 参考周期增益，‰/count，有限非负数；增大会提高灵敏度。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS
#define DBUS_PITCH_MOUSE_VIRTUAL_HOLD_MS 14U /* Pitch 缺帧保持，ms，可为 0；过小会在正常帧间提前降速。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS
#define DBUS_PITCH_MOUSE_VIRTUAL_DECAY_MS 14U /* Pitch 缺帧回中，ms，>0；增大会延长断续输入的拖尾。 */
#endif
#ifndef DBUS_PITCH_MOUSE_VIRTUAL_SIGN
#define DBUS_PITCH_MOUSE_VIRTUAL_SIGN 1 /* Pitch 鼠标方向，+1/-1；方向反时只改此项。 */
#endif

#ifndef DBUS_MOUSE_VIRTUAL_ENABLE
#define DBUS_MOUSE_VIRTUAL_ENABLE 1U /* 虚拟鼠标总开关，0/1；异常时可关闭鼠标控制。 */
#endif
#ifndef DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE
#define DBUS_MOUSE_VIRTUAL_OUTPUT_LIMIT_PERMILLE 100 /* 鼠标上限，1~1000‰；两轴满输入40rpm时仅鼠标目标速度限为4rpm。 */
#endif

_Static_assert(DBUS_MOUSE_VIRTUAL_ENABLE == 0U || DBUS_MOUSE_VIRTUAL_ENABLE == 1U,
               "virtual mouse switch must be 0 or 1");
_Static_assert(DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS > 0U &&
               (uint64_t)DBUS_MOUSE_VIRTUAL_REFERENCE_PERIOD_MS <= INT32_MAX,
               "mouse reference period must fit positive HAL half-range");
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
