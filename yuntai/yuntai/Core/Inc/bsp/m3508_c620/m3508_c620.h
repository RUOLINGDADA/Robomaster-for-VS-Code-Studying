/**
 * @file m3508_c620.h
 * @brief C620/M3508 标准 CAN 电流协议与一致反馈快照。
 *
 * 接收在 CAN ISR 解码，注册、快照及发送在任务上下文执行。
 * 协议源为 MyChassis1 的 C620 驱动及其 c620_can 文档；不处理 PID、轮序或输入模式。
 */
#ifndef BSP_M3508_C620_H
#define BSP_M3508_C620_H /* 防止重复包含。 */
#include "can.h"
#include <stdbool.h>
#include <stdint.h>

#define C620_M3508_MAX_DEVICES 8U /* 静态注册容量，个；支持电调 ID1~8。 */
#define C620_M3508_CURRENT_LIMIT_RAW 16384 /* 官方电流指令绝对上限，raw，约 20 A。 */
#define C620_M3508_FEEDBACK_BASE_ID 0x200U /* 标准反馈 ID=0x200+电调 ID。 */
#define C620_M3508_CONTROL_LOW_ID 0x200U /* ID1~4 的标准电流控制帧，每槽 2 字节。 */
#define C620_M3508_CONTROL_HIGH_ID 0x1FFU /* ID5~8 的标准电流控制帧。 */
#define C620_M3508_FRAME_BYTES 8U /* C620 标准数据帧长度，字节。 */

typedef struct {
  CAN_HandleTypeDef *hcan; /* 实际总线句柄；Init 值复制配置，指针须长期有效。 */
  uint8_t motor_id; /* 电调 ID1~8；同总线不可重复注册。 */
  uint32_t feedback_timeout_ms; /* HAL ms，新鲜度阈值，范围 1~INT32_MAX。 */
} C620_M3508_ConfigTypeDef;

typedef struct {
  uint16_t angle_raw; /* DATA[0:1] 大端单圈转子计数，0~8191。 */
  int16_t speed_rpm; /* DATA[2:3] 大端有符号转子 rpm，未做安装方向变换。 */
  int16_t current_raw; /* DATA[4:5] 大端反馈电流 raw，未做方向变换。 */
  uint8_t temperature_c; /* DATA[6] 温度，摄氏度，来源见模块协议说明。 */
  uint8_t status_raw; /* DATA[7] 原样保存；不同固件解释待确认，不作保护判断。 */
  uint32_t timestamp_ms; /* ISR 收到合法反馈时的 HAL ms，0 ms 也有效。 */
  uint32_t age_ms; /* 快照复制时的 HAL ms 年龄，允许短间隔回绕。 */
  bool received; /* 曾收到过合法帧；InvalidateFeedback 后清除。 */
  bool online; /* 快照时 received 且年龄未超时，与输出开关独立。 */
} C620_M3508_SnapshotTypeDef;

typedef struct {
  C620_M3508_ConfigTypeDef config; /* Init 后只读，注册期间不得直接修改。 */
  C620_M3508_SnapshotTypeDef feedback; /* ISR 写、任务通过 GetSnapshot 复制，不直接读多字段。 */
  int16_t target_current_raw; /* 所属任务写的待发电流 raw；关闭许可时清零。 */
  bool output_enabled; /* 所属任务控制；发送时仍独立复核反馈年龄。 */
  bool initialized; /* 已在静态注册表中占槽，DeInit 清除。 */
} C620_M3508_HandleTypeDef;

/**
 * @brief 注册一个静态或长期存活句柄，初始无反馈且零输出。
 * @param motor 调用者所有的句柄，不能用已经返回的栈内存。
 * @param config 总线、ID 和 HAL ms 期限，调用期间只读。
 * @retval true 注册完成；false 参数非法、重复注册或容量不足，句柄不变。
 * @note 任务调用；与 ISR 的注册表访问使用短 PRIMASK 临界区。
 */
bool C620_M3508_Init(C620_M3508_HandleTypeDef *motor, const C620_M3508_ConfigTypeDef *config);
/**
 * @brief 解除注册并清句柄，供初始化失败回滚。
 * @param motor 已注册句柄。
 * @retval None；空或未注册不操作。
 * @note 仅所属任务调用，内部短临界区；不会自动发送停机帧。
 */
void C620_M3508_DeInit(C620_M3508_HandleTypeDef *motor);
/**
 * @brief 复制一次完整反馈并计算新鲜度。
 * @param motor 已注册句柄。
 * @param snapshot 输出副本，失败不改输出。
 * @retval true 完成复制，未收到首帧时 online=false；false 参数非法。
 * @note 任务调用；短临界区保证角度、速度和时间来自同一帧。
 */
bool C620_M3508_GetSnapshot(const C620_M3508_HandleTypeDef *motor, C620_M3508_SnapshotTypeDef *snapshot);
/**
 * @brief 设置输出许可，只有真实新鲜反馈才可使能。
 * @param motor 所属电机。
 * @param enabled true 请求使能；false 同时清电流。
 * @retval true 请求接受；false 非法句柄或反馈不可用，后者保持零输出。
 * @note 任务调用；不发送，不等待。
 */
bool C620_M3508_SetOutputEnabled(C620_M3508_HandleTypeDef *motor, bool enabled);
/**
 * @brief 保存钳位到官方范围的电流指令。
 * @param motor 已注册句柄。
 * @param current_raw 原始值，按 [-16384,16384] 钳位。
 * @retval true 已保存；false 句柄非法。
 * @note 任务调用；发送入口仍需许可和反馈新鲜，不因写值自动使能。
 */
bool C620_M3508_SetCurrent(C620_M3508_HandleTypeDef *motor, int16_t current_raw);
/**
 * @brief 使 Bus-Off 前反馈失效并关闭电流。
 * @param motor 已注册句柄。
 * @retval None。
 * @note 任务调用；恢复后必须收到新帧再由任务重新使能，不能沿用旧反馈。
 */
void C620_M3508_InvalidateFeedback(C620_M3508_HandleTypeDef *motor);
/**
 * @brief 解码分发层已经取出的一帧，不再读取 FIFO。
 * @param hcan 来源总线，按句柄隔离同 ID。
 * @param header 只接受标准数据帧、8 字节、反馈 ID0x201~0x208。
 * @param data 8 字节负载，调用期间有效。
 * @retval true 更新对应注册电机；false 非法帧或未注册。
 * @note CAN ISR 调用，只记录数据与 HAL ms，不打印、不阻塞。
 */
bool C620_M3508_HandleRxMessage(CAN_HandleTypeDef *hcan, const CAN_RxHeaderTypeDef *header, const uint8_t *data);
/**
 * @brief 聚合同总线电流，在同一短临界区构建并提交 HAL。
 * @param hcan 目标总线。
 * @retval true 已提交或无设备；false 非法总线、Bus-Off、邮箱忙或 HAL 失败。
 * @note 任务调用；掉线和禁用槽位发零，不等待，不重试。HAL 接受不等于电调已执行。
 */
bool C620_M3508_SendAll(CAN_HandleTypeDef *hcan);
#endif /* BSP_M3508_C620_H */
