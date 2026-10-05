/**
  ******************************************************************************
  * @file    gm6020.h
  * @brief   RoboMaster GM6020 云台电机的 CAN 电流驱动接口。
  *
  * GM6020 的反馈帧使用标准 CAN ID 0x205~0x20B；电流控制帧使用
  * 0x1FE（电机 ID 1~4）或 0x2FE（电机 ID 5~7）。0x1FF/0x2FF 是
  * 手册中的电压控制帧，不能在已经配置电流环时误用。
  * 本模块负责协议字节解析、单圈编码器回绕、连续角度累计、在线超时和
  * 可选的软件角度状态；云台闭环控制、动态堵转判断和目标角度规划
  * 属于 app/gimbal 运行时，因为这些逻辑必须知道遥控方向和任务阶段。
  * 接收函数可在 CAN ISR 中调用，ISR 路径不打印串口、不阻塞；周期发送和
  * 诊断日志应放在 FreeRTOS 任务中完成。
  * 通俗理解：这个驱动只负责“听懂电机反馈、打包电流命令”，不负责决定云台想转到哪里。
  ******************************************************************************
  */

#ifndef GM6020_H
#define GM6020_H /* 防止 GM6020 接口被同一编译单元重复包含（避免类型和宏重复定义）。 */

#include "stm32f4xx_hal.h"

#if defined(HAL_CAN_MODULE_ENABLED)
#include "stm32f4xx_hal_can.h"
#else
typedef struct CAN_HandleTypeDef CAN_HandleTypeDef;
typedef struct CAN_RxHeaderTypeDef CAN_RxHeaderTypeDef;
#endif

#include <stdbool.h>
#include <stdint.h>

#define GM6020_FRAME_DLC 8U /* GM6020 反馈/控制 CAN 帧固定 8 字节（DATA[0]~DATA[7]）。 */
#define GM6020_FEEDBACK_ID_YAW 0x205U /* Yaw 默认反馈标准帧 ID（对应电机 ID 1）。 */
#define GM6020_FEEDBACK_ID_PITCH 0x206U /* Pitch 默认反馈标准帧 ID（对应电机 ID 2）。 */
#define GM6020_FEEDBACK_ID_MIN 0x205U /* 驱动接受的最小 GM6020 反馈 ID（电机 ID 1）。 */
#define GM6020_FEEDBACK_ID_MAX 0x20BU /* 手册规定的最大反馈 ID，电机 ID 7（闭区间）。 */
#define GM6020_CURRENT_CONTROL_ID_LOW 0x1FEU /* 电流控制帧：电机 ID 1~4（每个占两个字节）。 */
#define GM6020_CURRENT_CONTROL_ID_HIGH 0x2FEU /* 电流控制帧：电机 ID 5~7（每个占两个字节）。 */
#define GM6020_VOLTAGE_CONTROL_ID_LOW 0x1FFU /* 电压控制帧：电机 ID 1~4；本驱动不发送（当前使用电流环）。 */
#define GM6020_VOLTAGE_CONTROL_ID_HIGH 0x2FFU /* 电压控制帧：电机 ID 5~7；本驱动不发送（避免误切换控制模式）。 */
#define GM6020_ENCODER_COUNTS_PER_REV 8192U /* 单圈编码器计数，13 位范围（有效值 0~8191）。 */
#define GM6020_CURRENT_RAW_MIN (-16384) /* 手册电流给定原始值下限，对应约 -3 A（控制仍使用原始值）。 */
#define GM6020_CURRENT_RAW_MAX 16384 /* 手册电流给定原始值上限，对应约 +3 A（控制仍使用原始值）。 */
#define GM6020_FEEDBACK_TIMEOUT_MS 100U /* 超过该 HAL 毫秒数没有反馈即离线（任务必须走零输出）。 */
#define GM6020_MAX_DEVICE_COUNT 7U /* 手册支持的有效电机 ID 1~7，静态表最多注册 7 个（不动态分配）。 */

/* 将上限设为这两个值表示暂不启用软件角度边界；反馈和参数安全门仍然有效。 */
#define GM6020_ANGLE_LIMIT_DISABLED_MIN INT32_MIN /* 禁用边界时使用的最小哨兵（表示不比较下限）。 */
#define GM6020_ANGLE_LIMIT_DISABLED_MAX INT32_MAX /* 禁用边界时使用的最大哨兵（表示不比较上限）。 */

typedef enum {
  GM6020_STATE_UNINITIALIZED = 0,
  GM6020_STATE_OFFLINE,
  GM6020_STATE_ONLINE,
  GM6020_STATE_RUNNING,
  GM6020_STATE_DISABLED,
} Gm6020_StateTypeDef;

typedef enum {
  GM6020_LIMIT_NONE = 0,
  GM6020_LIMIT_MIN,
  GM6020_LIMIT_MAX
} Gm6020_LimitTypeDef;

typedef struct {
  bool valid; /* true 表示中心与两侧边界由实机确认；默认 false（未确认时不能闭环驱动）。 */
  int32_t center_angle_raw; /* 机械中位连续角度，单位编码器计数（本轴位置零点）。 */
  int32_t min_angle_raw; /* 较小连续角度安全边界，不假定它是左侧或下侧（由实机记录）。 */
  int32_t max_angle_raw; /* 较大连续角度安全边界，必须大于中位（否则标定无效）。 */
} Gm6020_CalibrationTypeDef;

typedef struct {
  uint16_t angle_raw;          /* 0~8191，电机当前单圈编码器值（转满一圈会回到 0）。 */
  int32_t angle_total_raw;     /* 处理回绕后的连续角度，单位为编码器计数（跨圈后仍连续增加）。 */
  int16_t speed_rpm;            /* DATA[2:3] 有符号转速，单位 rpm（协议大端字段）。 */
  int16_t current_raw;          /* DATA[4:5] 有符号反馈电流原始值，非安培值（仅供诊断）。 */
  uint8_t temperature_c;        /* DATA[6] 电机温度，单位摄氏度（原样记录）。 */
  uint8_t reserved_raw;        /* DATA[7] 原始保留字节；手册未定义通用错误码（仅供诊断）。 */
  uint32_t last_feedback_tick;  /* HAL_GetTick() 写入的最近反馈时间，单位 HAL ms（用于超时）。 */
} Gm6020_FeedbackTypeDef;

typedef struct {
  CAN_HandleTypeDef *hcan;     /* 该电机所在的 HAL CAN 外设，初始化时只保存指针（不拥有外设）。 */
  uint16_t feedback_id;        /* 标准反馈 ID，例如 0x205；同时决定控制帧槽位（ID 映射到两字节）。 */
  int8_t current_sign;         /* 控制器逻辑电流到电机物理电流的方向，必须为 +1/-1（决定发送电流符号）。 */
  int8_t speed_sign;            /* 电调反馈转速到连续角度逻辑方向的映射，必须为 +1/-1（可与 current_sign 不同）。 */
  uint32_t feedback_timeout_ms; /* 反馈超过该 HAL 毫秒数未更新即判定离线（输出走零）。 */
  int32_t mechanical_min_raw;  /* 连续角度软件下限，单位编码器计数（只报告边界状态）。 */
  int32_t mechanical_max_raw;  /* 连续角度软件上限，单位编码器计数（必须大于下限）。 */
  bool angle_reference_enabled; /* true 表示首帧按标定参考重建连续角度（把单圈值对齐到中心）。 */
  uint16_t angle_reference_single_raw; /* 标定中心对应的单圈角度，范围 0~8191（用于首帧回绕）。 */
  int32_t angle_reference_total_raw; /* 标定中心对应的连续角度，单位编码器计数（首帧输出起点）。 */
} Gm6020_ConfigTypeDef;

typedef struct {
  Gm6020_FeedbackTypeDef feedback; /* 从同一次反馈更新复制出的完整反馈快照（字段彼此一致）。 */
  Gm6020_StateTypeDef state;       /* 驱动状态快照，例如 ONLINE/RUNNING/OFFLINE（便于诊断）。 */
  Gm6020_LimitTypeDef limit;       /* 当前连续角度边界，不拦截上层闭环电流。 */
  int16_t target_current_raw;      /* 当前缓存的电流给定原始值，发送前仍会看使能（缓存不等于已发）。 */
  bool output_enabled;             /* true 表示允许控制帧发送目标电流（false 时槽位写零）。 */
  bool online;                     /* 收到过有效首帧且未超时；独立于输出开关状态（在线不代表在转）。 */
  bool feedback_received; /* 收到过首帧；时间戳为 0 ms 也可能是真实反馈（不能只看时间戳判断在线）。 */
} Gm6020_SnapshotTypeDef;

typedef struct {
  Gm6020_ConfigTypeDef config;       /* Init 时复制的总线、ID、超时和角度边界配置（不保存外部指针）。 */
  Gm6020_FeedbackTypeDef feedback;   /* CAN ISR 更新、任务通过快照接口读取的反馈（可能并发访问）。 */
  int16_t target_current_raw;         /* 任务写入的目标电流原始值，范围由宏钳位（发送前仍检查反馈新鲜度）。 */
  Gm6020_StateTypeDef state;          /* 驱动当前状态，ISR 和任务都会在安全路径更新（用于日志）。 */
  Gm6020_LimitTypeDef limit;          /* 当前边界状态；输出方向由 app/gimbal 过滤。 */
  bool initialized;                   /* true 表示句柄已经加入静态注册表（可参与 CAN 聚合）。 */
  bool output_enabled;                /* false 时聚合控制帧对应槽位强制发送零（防止残留电流）。 */
  bool angle_initialized;             /* 是否已收到首帧并建立连续角度初值（未建立时不能闭环）。 */
  /* ISR 更新反馈前后各改变一次，用于任务读取一致快照（避免字段来自不同帧）。 */
  volatile uint32_t feedback_sequence; /* 偶数稳定、奇数表示 ISR 正在写反馈（读者只接受偶数）。 */
} Gm6020_HandleTypeDef;

/**
 * @brief  注册一个 GM6020 句柄并清零运行状态。
 * @param  hmotor 调用方持有的句柄，不能为 NULL。
 * @param  config CAN 句柄、反馈 ID、超时、角度边界和可选标定参考；函数会复制配置内容。
 * @retval true 参数合法且注册成功。
 * @retval false 参数非法、同一 CAN 总线上的 ID 重复或静态注册表已满。
 * @note   本函数不启动 CAN，也不配置过滤器；初始化属于板级代码。
 */
bool Gm6020_Init(Gm6020_HandleTypeDef *hmotor,
                 const Gm6020_ConfigTypeDef *config);

/**
 * @brief  注销 GM6020 句柄。
 * @param  hmotor 已初始化句柄。
 * @retval true 注销成功；false 参数为空或句柄未注册。
 */
bool Gm6020_DeInit(Gm6020_HandleTypeDef *hmotor);

/**
 * @brief  设置目标电流缓存。
 * @param  hmotor 电机句柄。
 * @param  current_raw GM6020 电流给定原始值，自动钳位到 -16384~16384。
 * @retval true 已缓存；false 句柄无效。
 * @note   这里只改 RAM，边界命令过滤由 app/gimbal 完成。
 */
bool Gm6020_SetCurrent(Gm6020_HandleTypeDef *hmotor, int16_t current_raw);
/**
 * @brief  开关输出。
 * @param  hmotor 电机句柄。
 * @param  enabled false 会立即把目标电流清零并保持关闭。
 * @retval true 许可已设置；false 句柄无效或反馈离线。
 * @note   每轴由自己的任务写；驱动内部短临界区与 CAN ISR 同步状态。
 */
bool Gm6020_SetOutputEnabled(Gm6020_HandleTypeDef *hmotor, bool enabled);

/**
 * @brief  重新设置软件角度边界。
 * @param  hmotor 电机句柄。
 * @param  min_angle_raw 连续角度下限，单位编码器计数。
 * @param  max_angle_raw 连续角度上限，必须大于下限。
 * @retval true 设置成功；false 参数非法。
 * @note   这不是读取真实机械挡块位置；首次上电应先标定零点和边界。
 */
bool Gm6020_SetMechanicalLimit(Gm6020_HandleTypeDef *hmotor,
                               int32_t min_angle_raw,
                               int32_t max_angle_raw);
/**
 * @brief  处理一帧从 HAL FIFO 取出的反馈。
 * @param  hcan CAN 外设句柄。
 * @param  rx_header HAL 标准帧头。
 * @param  data 8 字节数据区。
 * @retval true 帧属于本驱动并已更新；false 帧不匹配。
 * @note   可在 CAN ISR 调用；只做解析和状态记录，不调用串口或阻塞 API。
 */
bool Gm6020_HandleRxMessage(CAN_HandleTypeDef *hcan,
                            const CAN_RxHeaderTypeDef *rx_header,
                            const uint8_t data[GM6020_FRAME_DLC]);

/**
 * @brief  从指定 CAN FIFO 取一帧并解析。
 * @param  hcan CAN 外设句柄。
 * @param  rx_fifo HAL_CAN_RX_FIFO0 或 HAL_CAN_RX_FIFO1。
 * @retval true 取到并处理了本驱动帧；false HAL 失败或 ID 不匹配。
 */
bool Gm6020_RxFifoCallback(CAN_HandleTypeDef *hcan, uint32_t rx_fifo);

/**
 * @brief  发送本 CAN 总线上已注册句柄的电流控制帧。
 * @param  hmotor 已初始化句柄。
 * @retval true HAL 发送成功；false 邮箱不足、参数非法或设备未注册。
 * @note   ID 1~4 聚合到 0x1FE，ID 5~7 聚合到 0x2FE；每个任务使用自己的句柄。
 *         构建与提交在一个很短的 PRIMASK 临界区完成，不等待邮箱或总线完成；
 *         返回 true 只说明 HAL 接受，不能解释为电调已经执行。
 */
bool Gm6020_Send(Gm6020_HandleTypeDef *hmotor);

/**
 * @brief  在任务上下文更新在线和软件限位状态。
 * @param  hmotor 电机句柄。
 * @param  now_tick 与反馈时间戳相同的毫秒时基；当前实现应传入 HAL_GetTick()，使用无符号差值处理回绕。
 * @retval true 处理完成；false 句柄无效。
 */
bool Gm6020_Process(Gm6020_HandleTypeDef *hmotor, uint32_t now_tick);

/**
 * @brief  读取一组来自同一次 CAN 反馈更新的状态快照。
 * @param  hmotor 已初始化的 GM6020 句柄。
 * @param  snapshot 输出快照，不保存内部指针。
 * @retval true  成功取得一致快照。
 * @retval false 句柄无效或 ISR 更新过于频繁导致本次复制失败。
 *
 * @note   CAN ISR 使用序列号包住反馈写入，任务不应直接读取句柄中的
 *         feedback/state 字段，否则一行日志可能混合两帧数据。
 */
bool Gm6020_GetSnapshot(const Gm6020_HandleTypeDef *hmotor,
                        Gm6020_SnapshotTypeDef *snapshot);

/**
 * @brief  复制一份反馈、在线和边界状态快照。
 * @param  hmotor 电机句柄。
 * @param  feedback 输出反馈结构体。
 * @retval true 复制成功；false 参数非法。
 */
bool Gm6020_GetFeedback(const Gm6020_HandleTypeDef *hmotor,
                        Gm6020_FeedbackTypeDef *feedback);

/**
 * @brief  查询电机是否在线。
 * @param  hmotor 电机句柄。
 * @retval true 最近收到反馈；false 未初始化或已超时。
 */
bool Gm6020_IsOnline(const Gm6020_HandleTypeDef *hmotor);

/**
 * @brief  获取当前限位原因。
 * @param  hmotor 电机句柄。
 * @retval GM6020_LIMIT_NONE、MIN 或 MAX；该值只表示当前连续角度是否越过
 *         配置边界，不锁存限位，也不代表机械堵转。
 */
Gm6020_LimitTypeDef Gm6020_GetLimit(const Gm6020_HandleTypeDef *hmotor);

#endif /* GM6020_H（防止 GM6020 接口被重复包含） */


