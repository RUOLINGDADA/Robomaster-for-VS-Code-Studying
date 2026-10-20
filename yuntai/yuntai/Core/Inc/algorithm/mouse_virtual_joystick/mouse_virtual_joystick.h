/**
 * @file mouse_virtual_joystick.h
 * @brief 按 DBUS 接收帧间隔将鼠标相对位移转换为跟手速度。
 *
 * 模块用浮点数保留小增量，输出为浮点千分比。调用者提供 DBUS 帧序号，只消费
 * 每个序号一次；新帧替换速度，零位移帧清零，重复调用推进缺帧回中。模块不访问 HAL、FreeRTOS、
 * CAN 或 UART，适合在任务上下文和主机替身中复用。
 */
#ifndef ALGORITHM_MOUSE_VIRTUAL_JOYSTICK_H
#define ALGORITHM_MOUSE_VIRTUAL_JOYSTICK_H /* 防止虚拟鼠标接口重复包含。 */

#include <stdbool.h>
#include <stdint.h>

/** 虚拟鼠标的内部阶段；仅用于诊断，不改变控制输出。 */
typedef enum {
  MOUSE_VIRTUAL_JOYSTICK_STATE_IDLE = 0U, /* 输出为零。 */
  MOUSE_VIRTUAL_JOYSTICK_STATE_HOLD,      /* 等待新帧时保持最后速度。 */
  MOUSE_VIRTUAL_JOYSTICK_STATE_DECAY,     /* 缺帧超时后线性回中。 */
  MOUSE_VIRTUAL_JOYSTICK_STATE_INPUT      /* 本周期消费了新的非零帧。 */
} MouseVirtualJoystick_StateTypeDef;

/** 单轴虚拟鼠标参数；增益使用千分比/count，时间使用 ms。 */
typedef struct {
  float gain_permille_per_count; /* 参考帧周期下的有限增益，‰/count；符号由任务配置决定。 */
  uint32_t hold_ms; /* 缺新帧时保持速度的时长，ms，可为 0；零位移新帧直接清零。 */
  uint32_t decay_ms; /* 缺帧保持结束后的回中时长，ms，>0；与 hold_ms 之和不超过 INT32_MAX。 */
} MouseVirtualJoystick_AxisConfigTypeDef;

/** 虚拟鼠标公共参数；X/Y 时序由各自轴配置控制。 */
typedef struct {
  MouseVirtualJoystick_AxisConfigTypeDef x; /* X 轴配置。 */
  MouseVirtualJoystick_AxisConfigTypeDef y; /* Y 轴配置。 */
  uint32_t reference_period_ms; /* 位移速度归一化参考周期，ms，1~INT32_MAX；首帧或同毫秒帧也用此值。 */
  int32_t output_limit_permille; /* 输出绝对值上限，单位 ‰；有效范围 1~1000。 */
} MouseVirtualJoystick_ConfigTypeDef;

/** 单轴运行状态；X/Y 分别计时，避免移动 X 时持续续期 Y。 */
typedef struct {
  float value_permille; /* 当前速度，‰；保留小数，仅所属任务写入。 */
  float anchor_permille; /* 回中起点，‰；保留小数，不随重复调用缩减。 */
  uint32_t last_event_ms; /* 本轴最后非零事件的帧接收时刻，单位 HAL ms；0 ms 也有效。 */
  bool has_event; /* 是否存在非零测速事件；新帧零值或回中结束后清除。 */
  MouseVirtualJoystick_StateTypeDef state; /* 本轴阶段；INPUT 仅持续一个 Update 调用。 */
} MouseVirtualJoystick_AxisTypeDef;

/** 算法状态；同一实例只能由一个任务上下文访问。 */
typedef struct {
  MouseVirtualJoystick_ConfigTypeDef config; /* 初始化后的只读配置。 */
  MouseVirtualJoystick_AxisTypeDef x; /* X 轴独立速度、事件时刻和回中阶段。 */
  MouseVirtualJoystick_AxisTypeDef y; /* Y 轴独立速度、事件时刻和回中阶段。 */
  uint32_t last_frame_sequence; /* 最近消费的 DBUS 合法帧序号；由调用者提供。 */
  uint32_t last_frame_timestamp_ms; /* 最近消费帧的 HAL ms 时间戳；用于诊断和时基检查。 */
  uint32_t last_update_ms; /* 最近成功更新时刻，单位 HAL ms；回退或半圈以上跳变清零。 */
  bool initialized; /* 配置检查成功后为 true。 */
  bool has_frame; /* 是否已经消费过帧序号。 */
  bool has_update; /* 是否建立更新时间基准；0 ms 不表示尚未初始化。 */
  MouseVirtualJoystick_StateTypeDef state; /* 汇总诊断阶段，优先级 INPUT > DECAY > HOLD > IDLE。 */
} MouseVirtualJoystick_HandleTypeDef;

/**
 * @brief 校验配置并初始化虚拟鼠标算法。
 * @param handle 算法状态存储区，必须保持有效直到任务结束。
 * @param config 配置副本；函数不会保存调用者的临时指针。
 * @retval true 配置有效并完成初始化。
 * @retval false 指针、增益、时间或输出参数无效；有 handle 时使其失效并清零。
 * @note 普通任务或主机测试上下文调用，不可与 Update 并发访问同一实例。
 */
bool MouseVirtualJoystick_Init(
    MouseVirtualJoystick_HandleTypeDef *handle,
    const MouseVirtualJoystick_ConfigTypeDef *config);

/**
 * @brief 清零虚拟速度并保留已校验配置。
 * @param handle 已初始化的算法状态。
 * @retval None。空指针安全返回。
 * @note DBUS 离线、快照无效或模块恢复时调用；下一帧按参考周期测速。
 */
void MouseVirtualJoystick_Reset(MouseVirtualJoystick_HandleTypeDef *handle);

/**
 * @brief 消费一帧鼠标增量并推进保持/回中状态。
 * @param handle 已初始化的算法状态。
 * @param now_ms 当前任务时刻，单位 HAL ms；必须与帧时间戳同源。
 * @param frame_sequence 合法 DBUS 帧序号；相同序号不重复消费，允许 uint32_t 回绕和向前跳号。
 * @param frame_timestamp_ms 合法 DBUS 帧接收时刻，单位 HAL ms。
 * @param mouse_x 新帧或本批合法帧的 X 相对位移代数和，单位 count。
 * @param mouse_y 新帧或本批合法帧的 Y 相对位移代数和，单位 count。
 * @param online true 表示帧仍在在线窗口内；false 立即清零。
 * @param virtual_x 输出 X 速度，单位 ‰；保留内部小数，不回写算法状态。
 * @param virtual_y 输出 Y 速度，单位 ‰；保留内部小数，不回写算法状态。
 * @retval true 输入有效并返回当前虚拟速度。
 * @retval false 离线、数值/时间异常或指针无效；非空输出写零，状态清零。
 * @note 仅任务上下文调用。重复任务周期仍推进回中，但不重复消费同一帧。
 * @note now_ms 须在取得快照后读取；禁止用未来帧时刻和较旧任务时刻相减。
 * @note 新帧用接收时间差计算速度；重复帧只推进缺帧回中，不重新消费位移。
 */
bool MouseVirtualJoystick_Update(
    MouseVirtualJoystick_HandleTypeDef *handle,
    uint32_t now_ms,
    uint32_t frame_sequence,
    uint32_t frame_timestamp_ms,
    int32_t mouse_x,
    int32_t mouse_y,
    bool online,
    float *virtual_x,
    float *virtual_y);

#endif /* ALGORITHM_MOUSE_VIRTUAL_JOYSTICK_H */
