/**
  ******************************************************************************
  * @file    c610_m2006.h
  * @brief   C610 电调与 M2006 电机的 RoboMaster CAN 驱动接口。
  *
  * 本模块负责设备注册、CAN 反馈解析、目标电流缓存、通信超时判断和
  * 同一 CAN 总线上的控制帧聚合发送。反馈接收入口可能运行在 CAN 中断，
  * 因此中断路径只更新短数据并记录时间，不执行控制器和日志等耗时工作。
  * 电机闭环、任务调度和业务故障处理由上层模块负责。
  ******************************************************************************
  */

#ifndef C610_M2006_H
#define C610_M2006_H /* 防止同一编译单元重复展开 C610/M2006 接口（避免定义重复）。 */

#include "stm32f4xx_hal.h"

/*
 * 当前 CubeMX 工程可能尚未生成 CAN HAL 文件（先声明句柄和帧头类型），
 * 让接口可以被 clangd/文档工具读取；启用 CAN 后由工程的 HAL 头提供完整定义。
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

#define C610_M2006_FRAME_DLC 8U /* C610 反馈和聚合控制帧固定为 8 字节（DATA[0]~DATA[7]）。 */
#define C610_M2006_MAX_DEVICE_COUNT 8U /* 静态注册表最多保存 8 个电调句柄（对应协议 ID 1~8）。 */
#define C610_M2006_MIN_DEVICE_ID 1U /* RoboMaster 电调编号从 1 开始（0 不是有效电机）。 */
#define C610_M2006_MAX_DEVICE_ID 8U /* C610/M2006 支持的最大电调编号（超出范围拒绝注册）。 */

/* RoboMaster 标准电机协议的聚合控制帧 ID（每个电机占连续两个数据字节）。 */
#define C610_M2006_CONTROL_ID_LOW 0x200U /* ID1~4 使用的聚合控制帧（DATA[0:7] 分别对应 1~4）。 */
#define C610_M2006_CONTROL_ID_HIGH 0x1FFU /* ID5~8 使用的聚合控制帧（DATA[0:7] 分别对应 5~8）。 */

/* 反馈帧 ID = 0x200 + 电调 ID，例如 ID=1 时为 0x201（标准帧 ID）。 */
#define C610_M2006_FEEDBACK_ID_BASE 0x200U /* 反馈 ID = 0x200 + 电调 ID（接收时按此公式匹配）。 */
#define C610_M2006_ENCODER_COUNTS_PER_REV 8192U /* 反馈机械角度范围 0~8191，对应一圈（13 位计数）。 */

/*
 * 官方手册 `参考文档/markdown/C610_-----------.md` 规定 C610/M2006 控制转矩
 * 电流范围为 [-10000, 10000] 原始值。这里按该协议范围钳位；不能复用
 * C620/3508 或 GM6020 的其它型号范围（不同电调不能共用限幅）。
 */
#define C610_M2006_CURRENT_RAW_MIN (-10000) /* M2006/C610 目标电流原始值下限（协议量，不是安培）。 */
#define C610_M2006_CURRENT_RAW_MAX 10000 /* M2006/C610 目标电流原始值上限（协议量，不是安培）。 */
#define C610_M2006_FEEDBACK_TIMEOUT_MS 100U /* 超过 100 ms 无反馈即视为离线（任务必须发零）。 */

typedef enum {
  C610_M2006_STATE_UNINITIALIZED = 0,
  C610_M2006_STATE_OFFLINE,
  C610_M2006_STATE_ONLINE,
  C610_M2006_STATE_DISABLED,
  C610_M2006_STATE_RUNNING
} C610_M2006_StateTypeDef;

typedef struct {
  uint16_t angle_raw;        /* DATA[0:1] 机械角度，驱动取低 13 位，单位计数（0~8191 一圈）。 */
  int16_t speed_rpm;         /* DATA[2:3] 有符号转速，单位 rpm（高字节在前）。 */
  int16_t current_raw;       /* DATA[4:5] 有符号实际转矩电流原始值（保留协议原始量）。 */
  uint8_t reserved_raw; /* DATA[6]：官方手册标为空，保留原始值供诊断（不参与保护判断）。 */
  uint8_t error_code;   /* DATA[7]：官方手册定义的电机错误码（原样保留，交给上层解释）。 */
  uint32_t last_feedback_tick; /* HAL_GetTick() 写入的最近反馈时间，单位 HAL ms（用于掉线判断）。 */
} C610_M2006_FeedbackTypeDef;

typedef struct {
  CAN_HandleTypeDef *hcan;       /* 该电调所在的 HAL CAN 外设句柄（初始化时只保存指针）。 */
  uint8_t motor_id;              /* C610 电调 ID，协议有效范围为 1~8（决定聚合帧槽位）。 */
  uint32_t feedback_timeout_ms;  /* 反馈超过该 HAL 毫秒数未更新即判定离线（安全路径发零）。 */
} C610_M2006_ConfigTypeDef;

typedef struct {
  C610_M2006_ConfigTypeDef config; /* Init 时复制的 CAN、ID 和超时配置（不保存外部指针）。 */
  C610_M2006_FeedbackTypeDef feedback; /* CAN ISR 更新、任务复制读取的反馈数据（同一对象可能并发访问）。 */
  int16_t target_current_raw;          /* 任务缓存的目标电流原始值，发送前已钳位（不等于已发出）。 */
  C610_M2006_StateTypeDef state;       /* 当前在线、运行或禁用状态（由 Process/发送路径更新）。 */
  bool initialized;                    /* true 表示句柄已经加入静态注册表（可参与聚合发送）。 */
  bool output_enabled;                 /* false 时聚合帧对应槽位强制发送零电流（即使目标缓存非零）。 */
  volatile bool feedback_received; /* ISR 写入、任务读取；至少收到一帧合法反馈（不能只看时间戳）。 */
  volatile uint32_t feedback_sequence; /* 偶数表示反馈稳定、奇数表示 ISR 正在写入（用于一致快照）。 */
} C610_M2006_HandleTypeDef;

/**
 * @brief  注册并初始化一个 C610/M2006 设备句柄。
 * @param  hmotor 由调用方提供的句柄存储，不能为 NULL。
 * @param  config  CAN 句柄、电调 ID 和超时参数。
 * @retval true   初始化成功并完成静态注册。
 * @retval false  参数非法、ID 重复、总线为空或注册表已满。
 *
 * @note   本函数不启动 CAN 外设，也不配置过滤器；这属于 CubeMX/板级层。
 */
bool C610_M2006_Init(C610_M2006_HandleTypeDef *hmotor,
                     const C610_M2006_ConfigTypeDef *config);

/**
 * @brief  注销一个已注册设备并清零其运行状态。
 * @param  hmotor 已初始化的设备句柄。
 * @retval true   注销成功。
 * @retval false  参数为空或句柄未注册。
 */
bool C610_M2006_DeInit(C610_M2006_HandleTypeDef *hmotor);

/**
 * @brief  设置目标电流原始值。
 * @param  hmotor 电机句柄。
 * @param  current_raw C610 CAN 协议中的有符号电流原始值。
 * @retval true   值已写入目标缓存。
 * @retval false  参数非法或设备未初始化。
 *
 * @note   本函数只更新 RAM 中的目标值；真正发送由
 *         C610_M2006_SendAll() 完成，因此适合在控制任务中调用。
 */
bool C610_M2006_SetCurrent(C610_M2006_HandleTypeDef *hmotor,
                           int16_t current_raw);

/**
 * @brief  设置输出使能状态。
 * @param  hmotor 电机句柄。
 * @param  enabled true 允许发送目标电流，false 强制本设备电流为零。
 * @retval true   状态已更新。
 * @retval false  参数非法或设备未初始化。
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
 *
 * @note   可从 `HAL_CAN_RxFifo0MsgPendingCallback` 调用；不要在这里阻塞。
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
 */
bool C610_M2006_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo);

/**
 * @brief  发送所有已注册设备的聚合控制帧。
 * @param  hcan 指定 CAN 总线句柄。
 * @retval true   没有发送错误，或该总线上没有设备。
 * @retval false  CAN 邮箱不足、句柄非法或 HAL 发送失败。
 *
 * @note   建议由固定周期任务调用。每条总线最多发送 0x200 和 0x1FF
 *         两帧，帧中的每两个字节对应一个电机 ID。函数在提交前检查反馈
 *         新鲜度，并在短 PRIMASK 临界区内完成槽位读取、构建和提交；不等待邮箱。
 */
bool C610_M2006_SendAll(CAN_HandleTypeDef *hcan);

/**
 * @brief  根据当前 tick 更新在线/离线状态。
 * @param  hmotor 电机句柄。
 * @param  now_tick 当前 FreeRTOS/HAL tick，调用者应在同一轮复用。
 * @retval true   状态检查完成。
 * @retval false  参数非法或设备未初始化。
 */
bool C610_M2006_Process(C610_M2006_HandleTypeDef *hmotor,
                        uint32_t now_tick);

/**
 * @brief  复制一份反馈快照。
 * @param  hmotor 电机句柄。
 * @param  feedback 输出结构体。
 * @retval true   复制成功。
 * @retval false  参数非法或设备未初始化。
 *
 * @note   接收中断可能同时更新句柄；实现使用短序列号重试取得同一帧，
 *         连续冲突时返回 false，调用者不能把失败输出当成真实反馈。
 */
bool C610_M2006_GetFeedback(const C610_M2006_HandleTypeDef *hmotor,
                            C610_M2006_FeedbackTypeDef *feedback);

/**
 * @brief  根据 HAL 毫秒时间判断反馈是否仍在线（只报告新鲜度，不保证多字段原子快照）。
 * @param  hmotor 已初始化的 C610/M2006 句柄。
 * @retval true 最近收到有效反馈；false 未初始化、从未收到反馈或超过超时。
 */
bool C610_M2006_IsOnline(const C610_M2006_HandleTypeDef *hmotor);

#endif /* C610_M2006_H（防止 C610/M2006 接口被重复包含） */
