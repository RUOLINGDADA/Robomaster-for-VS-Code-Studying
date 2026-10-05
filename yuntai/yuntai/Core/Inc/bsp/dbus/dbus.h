/**
 * @file dbus.h
 * @brief USART3 的 DJI DBUS 接收与一致快照（驱动只提供输入，不直接控制电机）。
 *
 * 协议参考官方 19.gimbal_task/remote_control，传输参考 hardware_dbm_uart_dma_dbus。
 * DMA ISR 复制完整帧到固定邮箱，任务解码；无 CRC，只能校验字段合理性，不能证明每一位都正确。
 */
#ifndef DBUS_H
#define DBUS_H /* 防止 DBUS 接口重复包含（避免类型重复定义）。 */
#include <stdbool.h>
#include <stdint.h>

#define DBUS_FRAME_LENGTH 18U /* DJI DBUS 固定帧长，单位字节（9 字节半帧不能解码）。 */
#define DBUS_CHANNEL_CENTER 1024 /* 官方通道中心原始值（减去它后回中为零）。 */
#define DBUS_CHANNEL_SPAN 660 /* 官方摇杆满量程偏移，单位原始计数（364~1684 对应 ±660）。 */
#define DBUS_OFFLINE_TIMEOUT_MS 100U /* 有效帧过期时间，单位 HAL ms（坏帧不能续期）。 */
#define DBUS_RETRY_PERIOD_MS 20U /* DMA 启动失败后的重试间隔，单位 HAL ms（不忙等、不停住其它任务）。 */
#define DBUS_SWITCH_UP 1U /* 官方拨杆上档编码（不能按自然顺序猜数值）。 */
#define DBUS_SWITCH_MIDDLE 3U /* 官方拨杆中档编码（中档是 3，不是 2）。 */
#define DBUS_SWITCH_DOWN 2U /* 官方拨杆下档编码（本轮只解码，不参与使能）。 */

typedef struct {
  int16_t channels[5]; /* CH0/1 右摇杆左右/上下，CH2/3 左摇杆，CH4 滚轮；已减 1024，单位原始计数。 */
  uint8_t switch_left; /* S1 左拨杆，参考工程定义为 DATA[5] bit6~7；1/3/2 为上/中/下，0 是接收器保留值。 */
  uint8_t switch_right; /* S2 右拨杆，参考工程定义为 DATA[5] bit4~5；1/3/2 为上/中/下，0 是接收器保留值。 */
  int16_t mouse_x; /* DATA[6~7] 小端有符号鼠标 X 增量（高字节保留符号）。 */
  int16_t mouse_y; /* DATA[8~9] 小端有符号鼠标 Y 增量（本轮不控制云台）。 */
  int16_t mouse_z; /* DATA[10~11] 小端有符号滚轮增量（不是摇杆 CH4）。 */
  uint8_t mouse_left; /* DATA[12]，0/1 为鼠标左键释放/按下（其它值拒绝整帧）。 */
  uint8_t mouse_right; /* DATA[13]，0/1 为鼠标右键释放/按下。 */
  uint16_t keyboard; /* DATA[14~15] 小端按键位图，bit0~15 为 W/S/A/D/Shift/Ctrl/Q/E/R/F/G/Z/X/C/V/B。 */
} Dbus_DataTypeDef;

typedef struct {
  Dbus_DataTypeDef data; /* 最近合法帧；离线时仅用于诊断（不能继续驱动）。 */
  uint32_t timestamp_ms; /* 合法帧 DMA 完成时的 HAL ms（不是任务打印或转发时间）。 */
  uint32_t age_ms; /* 当前 HAL 时间减接收时间，单位 ms（无符号相减支持回绕）。 */
  uint32_t valid_frames; /* 任务成功解码帧数，累计值（覆盖队列可能跳过旧帧）。 */
  uint32_t invalid_frames; /* 字段校验失败帧数，累计值（不会刷新 timestamp）。 */
  uint32_t resync_count; /* 启动/半帧导致重新建立边界次数（不是全都代表硬件故障）。 */
  uint32_t uart_errors; /* UART PE/FE/NE/ORE 故障次数（ISR 记录，任务读取快照）。 */
  uint32_t dma_errors; /* DMA 错误次数（不在 ISR 内阻塞重启）。 */
  uint32_t start_errors; /* DMA 启动/恢复失败次数（失败后继续限频重试）。 */
  bool valid; /* 曾收到一份合法帧（0 ms 也可能是有效接收时间）。 */
  bool online; /* 有合法新鲜帧且接收已同步、没有待恢复故障（读取成功不等于在线）。 */
} Dbus_SnapshotTypeDef;

/**
 * @brief 解码并校验一份 DBUS 帧（失败不改输出，不能靠坏数据刷新在线状态）。
 * @param frame 至少 18 字节的只读帧；通道按 11 位打包，鼠标/键盘按小端解析。
 * @param data 成功时写入解析结果。
 * @retval true 字段合法；false 空指针或字段非法。
 * @note 纯计算，无 HAL/RTOS 操作；无 CRC，范围校验不能发现所有损坏。
 */
bool Dbus_DecodeFrame(const uint8_t *frame, Dbus_DataTypeDef *data);
/**
 * @brief 初始化固定单帧邮箱并启动 USART3 双缓冲（先有复制落点，再允许 ISR 写入）。
 * @retval true 已启动；false 队列或 DMA 初始化失败，调用者可稍后重试。
 * @note USART3/DMA 必须先初始化；只由 DBUS 任务调用，不能在调度器启动前启用 FromISR 路径。
 */
bool Dbus_Init(void);
/**
 * @brief 非阻塞处理最新帧与恢复请求（ISR 不做解码、打印或等待）。
 * @param now_ms HAL_GetTick 的当前 ms，恢复重试使用同一时基。
 * @retval None 更新内部数据与接收状态。
 * @note 只由 DBUS 任务调用；HAL_DMA_Abort 的有界等待只发生在任务中。
 */
void Dbus_Process(uint32_t now_ms);
/**
 * @brief 获取一份完整数据与接收状态（短临界区保证字段来自同一状态）。
 * @param now_ms HAL 毫秒时间，不能传入 FreeRTOS Tick。
 * @param snapshot 输出快照，离线时 online=false。
 * @retval true 已复制；false 指针为空，输出不变。
 * @note 仅任务上下文；不返回内部缓冲区指针，不阻塞等待新帧。
 */
bool Dbus_GetSnapshot(uint32_t now_ms, Dbus_SnapshotTypeDef *snapshot);
/**
 * @brief USART3 专用 IRQ 钩子（只记录故障或 IDLE 边界，不进入 HAL 单缓冲接收状态机）。
 * @retval None；错误只请求任务恢复。
 * @note 仅 USART3 ISR 调用，优先级须为 5 或更低，不允许打印或普通 RTOS API。
 */
void Dbus_USART3_IRQHandler(void);
#endif /* DBUS_H */
