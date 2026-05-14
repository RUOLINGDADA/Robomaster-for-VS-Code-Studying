#ifndef C620_H
#define C620_H

#include "device_base.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
// ===================== 官方参数定义（100%对齐说明书） =====================
#define C620_MAX_CURRENT_RAW 16384      // 最大电流原始值（对应20A）
#define C620_MIN_CURRENT_RAW -16384     // 最小电流原始值（对应-20A）
#define C620_MAX_CURRENT_A 20.0f        // 最大电流(A)
#define C620_OVER_TEMP_WARN 125         // 过热警告阈值(℃)
#define C620_OVER_TEMP_CRIT 180         // 过热致命阈值(℃)
#define C620_COMM_TIMEOUT_MS 100        // CAN通信超时时间(ms)
#define C620_STALL_CURRENT_RAW 8192     // 堵转电流阈值(对应10A)
#define C620_STALL_TIME_MS 500          // 堵转持续判定时间(ms)
#define C620_TEMP_WARN_SHUTDOWN_ENABLE 1 // 温度警告时MCU侧主动关闭输出
#define C620_MAX_ESC_COUNT 8            // 单CAN总线最大电调数量
#define C620_CAN_CTRL_ID_BASE 0x200     // 控制帧ID（ESC 1-4）
#define C620_CAN_CTRL_ID_EXT 0x1FF      // 控制帧ID（ESC 5-8）
#define C620_CAN_FEEDBACK_ID_BASE 0x200 // 反馈帧ID基址（+ ESC ID）
#define C620_ANGLE_MAX_RAW 8191         // 机械角度最大原始值(0~8191)
#define C620_ANGLE_TO_DEG(raw) ((float)(raw) * 360.0f / 8191.0f)
#define C620_ANGLE_TO_RAD(raw) ((float)(raw) * 6.28318530f / 8191.0f)

#define C620_CURRENT_A_TO_RAW(current_a)                                      \
  ((int16_t)((current_a) * ((float)C620_MAX_CURRENT_RAW / C620_MAX_CURRENT_A)))

// ===================== 官方状态枚举 =====================
typedef enum {
  C620_STATE_INIT = 0,    // 上电自检状态
  C620_STATE_STOP,        // 停止状态（无输出，等待启动命令）
  C620_STATE_STANDBY,     // 待机状态（绿灯闪ID，无输出）
  C620_STATE_RUNNING,     // 运行状态（绿灯闪ID，有输出）
  C620_STATE_STALL,       // 堵转状态（电流大、转速低持续超时）
  C620_STATE_WARNING,     // 警告状态（橙灯闪烁，仍有输出）
  C620_STATE_ERROR,       // 错误状态（红灯闪烁/常亮，强制停止输出）
  C620_STATE_CALIBRATING, // 电机校准状态（绿灯快闪）
  C620_STATE_ID_SETTING   // ID设置状态（橙灯闪烁）
} C620_StateTypeDef;

// ===================== 官方错误码（CAN反馈DATA[7]） =====================
typedef enum {
  C620_ERROR_NONE = 0,             // 无异常
  C620_ERROR_MOTOR_EEPROM = 1,     // 无法访问电机存储芯片（开机自检）
  C620_ERROR_OVER_VOLTAGE = 2,     // 供电电压过高（开机自检）
  C620_ERROR_PHASE_MISSING = 3,    // 电机三相线未接入
  C620_ERROR_ENCODER_LOST = 4,     // 位置传感器数据丢失
  C620_ERROR_OVER_TEMP_CRIT = 5,   // 电机温度≥180℃（致命错误）
  C620_ERROR_CALIBRATION_FAIL = 7, // 电机校准失败
  C620_ERROR_OVER_TEMP_WARN = 8    // 电机温度≥125℃（警告）
} C620_OfficialErrorTypeDef;

// ===================== MCU端扩展错误码 =====================
typedef enum {
  C620_EXT_ERROR_NONE = 0,
  C620_EXT_ERROR_COMM_TIMEOUT = 1 << 0, // CAN通信超时
  C620_EXT_ERROR_STALL = 1 << 1,        // 电机堵转
  C620_EXT_ERROR_OVER_CURRENT = 1 << 2, // 电流超过20A持续
  C620_EXT_ERROR_OVER_TEMP = 1 << 3     // MCU端检测到过热
} C620_ExtErrorTypeDef;

// ===================== 警告类型（不停止输出） =====================
typedef enum {
  C620_WARNING_NONE = 0,
  C620_WARNING_OVER_TEMP = 1 << 0,   // 温度≥125℃
  C620_WARNING_ID_CONFLICT = 1 << 1, // CAN总线ID冲突
  C620_WARNING_PWM_INVALID = 1 << 2  // PWM输入无效（PWM模式用）
} C620_WarningTypeDef;

// ===================== 完整错误状态结构体 =====================
typedef struct {
  C620_OfficialErrorTypeDef official; // 电调硬件返回的错误
  C620_ExtErrorTypeDef ext;           // MCU检测的扩展错误
} C620_ErrorTypeDef;

// ===================== 命令ID枚举 =====================
typedef enum {
  C620_CMD_SET_CURRENT = 0, // 设置目标转矩电流（核心控制命令）
  C620_CMD_ENABLE_OUTPUT,   // 使能电机输出
  C620_CMD_DISABLE_OUTPUT,  // 禁用电机输出
  C620_CMD_GET_ANGLE,       // 获取转子机械角度
  C620_CMD_GET_SPEED,       // 获取转子转速(rpm)
  C620_CMD_GET_CURRENT,     // 获取实际转矩电流
  C620_CMD_GET_TEMP,        // 获取电机温度(℃)
  C620_CMD_RESET_ERROR,     // 重置所有错误状态
  C620_CMD_COUNT            // 命令总数
} C620_CmdIDTypeDef;

// 前向声明
typedef struct C620_HandleTypeDef C620_HandleTypeDef;

// ===================== 回调函数表 =====================
typedef struct {
  // 主状态变化回调
  void (*on_state_change)(C620_HandleTypeDef *hc620,
                          C620_StateTypeDef old_state,
                          C620_StateTypeDef new_state);

  // 官方硬件错误回调
  void (*on_official_error)(C620_HandleTypeDef *hc620,
                            C620_OfficialErrorTypeDef error);

  // MCU扩展错误回调
  void (*on_ext_error)(C620_HandleTypeDef *hc620, C620_ExtErrorTypeDef error);

  // 警告回调（不停止输出）
  void (*on_warning)(C620_HandleTypeDef *hc620, C620_WarningTypeDef warning);

  // 数据更新回调（每次收到CAN反馈触发，1KHz）
  void (*on_data_update)(C620_HandleTypeDef *hc620);

  // 堵转事件回调
  void (*on_stall)(C620_HandleTypeDef *hc620);

  // 通信恢复回调
  void (*on_comm_recover)(C620_HandleTypeDef *hc620);
} C620_CallbackTypeDef;

typedef struct {
  const char *name;
  uint8_t esc_id;
  CAN_HandleTypeDef *hcan;
} C620_ConfigTypeDef;

// ===================== C620电调句柄结构体 =====================
struct C620_HandleTypeDef {
  DeviceBase base; // 继承设备基类（必须第一个成员）

  // 硬件配置
  CAN_HandleTypeDef *hcan; // CAN总线句柄
  uint8_t esc_id;          // 电调ID(1-8)

  // 核心状态
  C620_StateTypeDef state;     // 主状态机
  C620_ErrorTypeDef error;     // 错误状态
  C620_WarningTypeDef warning; // 警告状态

  // 反馈数据（100%对齐官方CAN协议）
  uint16_t mechanical_angle; // 转子机械角度(0~8191对应0~360°)
  uint16_t last_mechanical_angle; // 上一次转子机械角度
  int16_t actual_speed;      // 转子转速(rpm)
  int16_t actual_current;    // 实际转矩电流(-16384~16384)
  uint8_t actual_temp;       // 电机温度(℃)

  // 控制数据
  int16_t target_current; // 目标转矩电流（C620只接收电流指令）
  bool enable_output;     // 输出使能标志
  bool feedback_timeout_enable; // 是否启用反馈超时保护

  // 计时变量
  uint32_t last_rx_time;     // 最后一次CAN反馈时间
  uint32_t stall_start_time; // 堵转开始时间

  // 异步事件标志（ISR置位，主循环消费）
  volatile bool data_updated;
  volatile bool comm_recovered;

  // 回调函数表
  C620_CallbackTypeDef callbacks;

  // 内部函数指针
  Device_StatusTypeDef (*process_loop)(C620_HandleTypeDef *hc620);
};

// ===================== 对外接口函数 =====================
/**
 * @brief 创建C620电调实例
 * @param name 设备名（调试用）
 * @param esc_id 电调ID(1-8)
 * @param hcan CAN总线句柄
 * @return 电调句柄指针，失败返回NULL
 */
C620_HandleTypeDef *C620_Create(const char *name, uint8_t esc_id,
                                CAN_HandleTypeDef *hcan);

/**
 * @brief 绑定一个静态分配的C620句柄
 * @param hc620 调用方提供的句柄存储
 * @param config 不变配置
 * @return 操作状态
 */
Device_StatusTypeDef C620_Init(C620_HandleTypeDef *hc620,
                               const C620_ConfigTypeDef *config);

/**
 * @brief 销毁C620电调实例
 * @param hc620 电调句柄
 * @return 操作状态
 */
Device_StatusTypeDef C620_Destroy(C620_HandleTypeDef *hc620);

/**
 * @brief 注册回调函数
 * @param hc620 电调句柄
 * @param callbacks 回调函数表
 * @return 操作状态
 */
Device_StatusTypeDef
C620_RegisterCallback(C620_HandleTypeDef *hc620,
                      const C620_CallbackTypeDef *callbacks);

/**
 * @brief 设置反馈超时保护
 * @param hc620 电调句柄
 * @param enable true=超时停输出，false=允许开环测试持续发送
 * @return 操作状态
 */
Device_StatusTypeDef C620_SetFeedbackTimeoutEnable(C620_HandleTypeDef *hc620,
                                                   bool enable);

/**
 * @brief 执行电调命令
 * @param hc620 电调句柄
 * @param cmd_id 命令ID
 * @param arg 命令参数
 * @return 操作状态
 */
Device_StatusTypeDef C620_ExecuteCommand(C620_HandleTypeDef *hc620,
                                         C620_CmdIDTypeDef cmd_id, void *arg);

/**
 * @brief 电调主循环处理函数（1ms调用一次）
 * @param hc620 电调句柄
 * @return 操作状态
 */
Device_StatusTypeDef C620_Process_Loop(C620_HandleTypeDef *hc620);

/**
 * @brief CAN接收中断回调函数（在HAL_CAN_RxFifo0MsgPendingCallback中调用）
 * @param hcan CAN总线句柄
 * @param RxFifo 接收FIFO编号
 */
bool C620_CAN_Rx_Callback(CAN_HandleTypeDef *hcan, uint32_t RxFifo);

/**
 * @brief 处理已经由上层读取出的CAN接收帧
 * @param hcan CAN总线句柄
 * @param rx_header 接收帧头
 * @param data 接收数据
 * @return true=C620反馈帧已处理，false=非C620帧或无匹配实例
 */
bool C620_HandleCanRxMessage(CAN_HandleTypeDef *hcan,
                             const CAN_RxHeaderTypeDef *rx_header,
                             const uint8_t data[8]);

/**
 * @brief 全局CAN发送函数（1ms调用一次，放在SysTick中断中）
 * @return 操作状态
 */
Device_StatusTypeDef C620_Send_All_Commands(void);

#endif // C620_H
