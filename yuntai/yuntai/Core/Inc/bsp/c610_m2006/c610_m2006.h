/**
 * @file c610_m2006.h
 * @brief C610/M2006 CAN 反馈与电流接口。
 *
 * 协议来自 C610 手册：反馈 0x201~0x208，控制 0x200/0x1FF，标准帧 8 字节。
 * CAN ISR 解码大端字段并展开连续角度；所属任务缓存电流、检查 HAL ms 年龄。
 * 任务在短临界区构建并提交聚合帧。过期槽位为零，不等待总线完成。
 * 本模块不启动 CAN，不运行角度闭环，不检测卡弹。
 */

#ifndef C610_M2006_H
#define C610_M2006_H /* 防止重复包含。避免定义重复。 */

#include "stm32f4xx_hal.h"

/*
 * 当前 CubeMX 工程可能尚未生成 CAN HAL 文件。先声明句柄和帧头类型，
 * 让接口可以被 clangd/文档工具读取。启用 CAN 后由工程的 HAL 头提供完整定义。
 * 驱动实现真正编译前必须满足 HAL_CAN_MODULE_ENABLED 和 CAN HAL 源文件已加入。
 */
#if defined(HAL_CAN_MODULE_ENABLED)
#include "stm32f4xx_hal_can.h"
#else
typedef struct CAN_HandleTypeDef CAN_HandleTypeDef;
typedef struct CAN_RxHeaderTypeDef CAN_RxHeaderTypeDef;
#endif
#include <stdbool.h>
#include <stdint.h>

#define C610_M2006_FRAME_DLC 8U /* C610 反馈和聚合控制帧固定为 8 字节。DATA[0]~DATA[7]。 */
#define C610_M2006_MAX_DEVICE_COUNT 8U /* 静态注册表最多保存 8 个电调句柄。对应协议 ID 1~8。 */
#define C610_M2006_MIN_DEVICE_ID 1U /* RoboMaster 电调编号从 1 开始。0 不是有效电机。 */
#define C610_M2006_MAX_DEVICE_ID 8U /* C610/M2006 支持的最大电调编号。超出范围拒绝注册。 */

/* RoboMaster 标准电机协议的聚合控制帧 ID。每个电机占连续两个数据字节。 */
#define C610_M2006_CONTROL_ID_LOW 0x200U /* ID1~4 使用的聚合控制帧。DATA[0:7] 分别对应 1~4。 */
#define C610_M2006_CONTROL_ID_HIGH 0x1FFU /* ID5~8 使用的聚合控制帧。DATA[0:7] 分别对应 5~8。 */

/* 反馈帧 ID = 0x200 + 电调 ID，例如 ID=1 时为 0x201。标准帧 ID。 */
#define C610_M2006_FEEDBACK_ID_BASE 0x200U /* 反馈 ID = 0x200 + 电调 ID。接收时按此公式匹配。 */
#define C610_M2006_ENCODER_COUNTS_PER_REV 8192U /* 反馈机械角度范围 0~8191，对应一圈。13 位计数。 */

/*
 * C610 无刷电机调速器使用说明 规定 C610/M2006 控制转矩
 * 电流范围为 [-10000, 10000] 原始值。这里按该协议范围钳位。不能复用
 * C620/3508 或 GM6020 的其它型号范围。不同电调不能共用限幅。
 */
#define C610_M2006_CURRENT_RAW_MIN (-10000) /* M2006/C610 目标电流原始值下限。协议量，不是安培。 */
#define C610_M2006_CURRENT_RAW_MAX 10000 /* M2006/C610 目标电流原始值上限。协议量，不是安培。 */
#define C610_M2006_FEEDBACK_TIMEOUT_MS 100U /* 超过 100 ms 无反馈即视为离线。任务必须发零。 */

typedef enum {
  C610_M2006_STATE_UNINITIALIZED = 0, /* 句柄尚未注册。 */
  C610_M2006_STATE_OFFLINE, /* 从未收到反馈或反馈过期；发送槽位为零。 */
  C610_M2006_STATE_ONLINE, /* 新鲜反馈可用；不代表输出已使能。 */
  C610_M2006_STATE_DISABLED, /* 输出许可关闭；发送零电流。 */
  C610_M2006_STATE_RUNNING /* 缓存非零电流且许可有效；不证明电机正在转。 */
} C610_M2006_StateTypeDef;

typedef struct {
  uint16_t angle_raw;        /* DATA[0:1] 机械角度，驱动取低 13 位，单位计数。0~8191 一圈。 */
  int64_t angle_total_raw; /* 每帧展开的电机轴连续计数。任务必须复制快照读取 64 位字段。 */
  int16_t speed_rpm;         /* DATA[2:3] 有符号转速，单位 rpm。高字节在前。 */
  int16_t current_raw;       /* DATA[4:5] 有符号实际转矩电流原始值。保留协议原始量。 */
  uint8_t reserved_raw; /* DATA[6]：官方手册标为空，保留原始值供诊断。不参与保护判断。 */
  uint8_t error_code;   /* DATA[7]：官方手册定义的电机错误码。原样保留，交给上层解释。 */
  uint32_t last_feedback_tick; /* HAL_GetTick() 写入的最近反馈时间，单位 HAL ms。用于掉线判断。 */
} C610_M2006_FeedbackTypeDef;

typedef struct {
  CAN_HandleTypeDef *hcan;       /* 该电调所在的 HAL CAN 外设句柄。初始化时只保存指针。 */
  uint8_t motor_id;              /* C610 电调 ID，协议有效范围为 1~8。决定聚合帧槽位。 */
  uint32_t feedback_timeout_ms;  /* 反馈超过该 HAL 毫秒数未更新即判定离线。安全路径发零。 */
} C610_M2006_ConfigTypeDef;

typedef struct {
  C610_M2006_ConfigTypeDef config; /* Init 时复制的 CAN、ID 和超时配置。仍持有外部 CAN 句柄指针。 */
  C610_M2006_FeedbackTypeDef feedback; /* CAN ISR 更新、任务复制读取的反馈数据。同一对象可能并发访问。 */
  int16_t target_current_raw;          /* 任务缓存的目标电流原始值，发送前已钳位。不等于已发出。 */
  C610_M2006_StateTypeDef state;       /* 当前在线、运行或禁用状态。由 Process/发送路径更新。 */
  bool initialized;                    /* true 表示句柄已经加入静态注册表。可参与聚合发送。 */
  bool output_enabled;                 /* false 时聚合帧对应槽位强制发送零电流。即使目标缓存非零。 */
  volatile bool feedback_received; /* ISR 写入、任务读取。至少收到一帧合法反馈。不能只看时间戳。 */
  volatile uint32_t feedback_sequence; /* 偶数表示稳定、奇数表示 ISR 正在写入。volatile 仅保证可见性；序列号与屏障保证快照。 */
} C610_M2006_HandleTypeDef;

/**
 * @brief  注册并初始化一个 C610/M2006 设备句柄。
 * @param  hmotor 由调用方提供的句柄存储，不能为 NULL。
 * @param  config  CAN 句柄、电调 ID 和超时参数。
 * @retval true   初始化成功并完成静态注册。
 * @retval false  参数非法、ID 重复、总线为空或注册表已满。
 * @note   本函数不启动 CAN 外设，也不配置过滤器。这属于 CubeMX/板级层。
 * @note 仅所属任务初始化。调用者串行化注册操作，注册时保持输出关闭。
 */
bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config);

/**
 * @brief  注销一个已注册设备并清零其运行状态。
 * @param  hmotor 已初始化的设备句柄。
 * @retval true   注销成功。
 * @retval false  参数为空或句柄未注册。
 * @note 仅所属任务调用。先提交零电流，并屏蔽 CAN 接收中断后注销。
 */
bool C610_M2006_DeInit(C610_M2006_HandleTypeDef *hmotor);

/**
 * @brief  设置目标电流原始值。
 * @param  hmotor 电机句柄。
 * @param  current_raw C610 CAN 协议中的有符号电流原始值。
 * @retval true   值已写入目标缓存。
 * @retval false  参数非法或设备未初始化。
 * @note   本函数只更新 RAM 中的目标值。真正发送由
 * C610_M2006_SendAll() 完成，因此适合在控制任务中调用。
 * @note 仅所属控制任务调用；ISR 不设置目标。
 */
bool C610_M2006_SetCurrent(C610_M2006_HandleTypeDef *hmotor,
                           int16_t current_raw);

/**
 * @brief  设置输出使能状态。
 * @param  hmotor 电机句柄。
 * @param  enabled true 允许发送目标电流，false 强制本设备电流为零。
 * @retval true   状态已更新。
 * @retval false  参数非法或设备未初始化。
 * @note 仅所属控制任务调用；ISR 不改变输出许可。
 */
bool C610_M2006_SetOutputEnabled(C610_M2006_HandleTypeDef *hmotor,
                                 bool enabled);

/**
 * @brief  处理一帧已经从 HAL CAN FIFO 取出的接收数据。
 * @param  hcan CAN 外设句柄。
 * @param  rx_header HAL 接收帧头。
 * @param  data 8 字节 CAN 数据区。
 * @retval true   该帧属于已注册设备并已更新反馈。
 * @retval false  该帧不是本驱动支持的反馈帧。
 * @note   可从 `HAL_CAN_RxFifo0MsgPendingCallback` 调用。不要在这里阻塞。
 * @note 仅由串行化的 CAN 接收路径调用；通常位于 CAN ISR。
 */
bool C610_M2006_HandleRxMessage(CAN_HandleTypeDef *hcan,
                                const CAN_RxHeaderTypeDef *rx_header,
                                const uint8_t data[C610_M2006_FRAME_DLC]);

/**
 * @brief  从硬件 CAN FIFO 取帧并交给本驱动解析。
 * @param  hcan CAN 外设句柄。
 * @param  rx_fifo HAL CAN FIFO 编号，例如 CAN_RX_FIFO0。
 * @retval true   取帧成功且属于本驱动设备。
 * @retval false  HAL 取帧失败或帧不匹配。
 * @note 仅 CAN ISR 调用。多个驱动共享 FIFO 时使用统一分发入口。
 */
bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo);

/**
 * @brief  发送所有已注册设备的聚合控制帧。
 * @param  hcan 指定 CAN 总线句柄。
 * @retval true   没有发送错误，或该总线上没有设备。
 * @retval false  CAN 邮箱不足、句柄非法或 HAL 发送失败。
 * @note   建议由固定周期任务调用。每条总线最多发送 0x200 和 0x1FF
 * 两帧，帧中的每两个字节对应一个电机 ID。函数在提交前检查反馈
 * 新鲜度，并在短 PRIMASK 临界区内完成槽位读取、构建和提交。不等待邮箱。
 * @note 仅任务调用。每条总线的槽位构建与提交使用同一短临界区。
 */
bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan);

/**
 * @brief  按 HAL ms 更新在线状态。
 * @param  hmotor 电机句柄。
 * @param  now_tick HAL_GetTick() 的当前 ms。不能传 FreeRTOS Tick。
 * @retval true   状态检查完成。
 * @retval false  参数非法或设备未初始化。
 * @note 仅所属任务调用；时间必须与 CAN ISR 的 HAL ms 同源。
 */
bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick);

/**
 * @brief  复制一份反馈快照。
 * @param  hmotor 电机句柄。
 * @param  feedback 输出结构体。
 * @retval true   复制成功。
 * @retval false  参数非法、未初始化或复制期间连续冲突。
 * @note   接收中断可能同时更新句柄。实现使用短序列号重试取得同一帧，
 * 连续冲突时返回 false，调用者不能把失败输出当成真实反馈。
 * @note 仅任务读取。失败输出不可用于控制；快照本身不保证反馈新鲜。
 */
bool C610_M2006_GetFeedback(const C610_M2006_HandleTypeDef *hmotor,
                            C610_M2006_FeedbackTypeDef *feedback);

/**
 * @brief  根据 HAL 毫秒时间判断反馈是否仍在线。只报告新鲜度，不保证多字段原子快照。
 * @param  hmotor 已初始化的 C610/M2006 句柄。
 * @retval true 最近收到有效反馈。false 未初始化、从未收到反馈或超过超时。
 * @note 仅任务查询新鲜度；需要多个反馈字段时读取 GetFeedback 快照。
 */
bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor);

#endif /* C610_M2006_H。防止 C610/M2006 接口被重复包含。 */
