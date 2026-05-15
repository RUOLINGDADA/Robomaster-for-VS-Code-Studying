#ifndef C620_H
#define C620_H

#include "device_base.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
// ===================== 官方参数定义（100%对齐说明书） =====================
// 这些宏不是“魔法数字”，而是把说明书里的协议、电气限制、安全阈值固化成代码。
// 工业项目里建议把硬件协议常量集中放在头文件，便于审查和追溯来源。
// 宏在预处理阶段替换，不占 RAM；但宏没有类型检查，所以只适合放纯常量/简单换算。
#define C620_MAX_CURRENT_RAW 16384      // 最大电流原始值（对应20A）
#define C620_MIN_CURRENT_RAW -16384     // 最小电流原始值（对应-20A）
#define C620_MAX_CURRENT_A 20.0f        // 最大电流(A)
#define C620_OVER_TEMP_WARN 125         // 过热警告阈值(℃)
#define C620_OVER_TEMP_CRIT 180         // 过热致命阈值(℃)
#define C620_COMM_TIMEOUT_MS 100        // CAN通信超时时间(ms)
#define C620_STALL_CURRENT_RAW 8192     // 堵转电流阈值(对应10A)
#define C620_STALL_TIME_MS 500          // 堵转持续判定时间(ms)
#define C620_OVER_CURRENT_TIME_MS 100   // 过流持续判定时间(ms)
#define C620_TEMP_WARN_SHUTDOWN_ENABLE 1 // 温度警告时MCU侧主动关闭输出
#define C620_MAX_ESC_COUNT 8            // 单CAN总线最大电调数量
#define C620_CAN_CTRL_ID_BASE 0x200     // 控制帧ID（ESC 1-4）：4个电调用同一帧的不同字节槽
#define C620_CAN_CTRL_ID_EXT 0x1FF      // 控制帧ID（ESC 5-8）：高ID组使用另一帧
#define C620_CAN_FEEDBACK_ID_BASE 0x200 // 反馈帧ID基址（+ ESC ID），ID1反馈就是0x201
#define C620_ANGLE_MAX_RAW 8191         // 机械角度最大原始值(0~8191)
#define C620_ANGLE_TO_DEG(raw) ((float)(raw) * 360.0f / 8191.0f)
#define C620_ANGLE_TO_RAD(raw) ((float)(raw) * 6.28318530f / 8191.0f)

#define C620_CURRENT_A_TO_RAW(current_a)                                      \
  ((int16_t)((current_a) * ((float)C620_MAX_CURRENT_RAW / C620_MAX_CURRENT_A)))
// 上面的换算宏里显式写 float，是为了避免整数除法把比例截断。
// 真实控制系统里更推荐把单位换算集中封装，避免“raw值”和“物理量”混用。

// ===================== 官方状态枚举 =====================
// 状态机是嵌入式安全代码的核心：任何输出都必须受 state + error 双重约束。
// 不要让“发送电流”散落在业务代码里，否则异常路径很难保证全部停机。
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
// 这些值来自电调固件，驱动不能随便重编号。
// DATA[7] 是“设备自己报告的错误”，和 MCU 侧检测的超时/堵转分开存储。
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
// 这里用 bit mask，而不是普通互斥枚举：一个设备可能同时有通信超时和过温。
// 1 << n 表示每个错误占一个 bit，组合时用 |，清除时用 &~。
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
// official 和 ext 分开，是为了保留故障来源：
// official 说明电调硬件/固件认为自己异常；
// ext 说明 MCU 根据时间、电流、速度等外部逻辑判断异常。
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
// 回调函数表里要用 C620_HandleTypeDef *，但完整结构体还在后面。
// 先告诉编译器“有这样一个类型”，指针大小已知，所以可以提前使用。
typedef struct C620_HandleTypeDef C620_HandleTypeDef;

// ===================== 回调函数表 =====================
// 回调是驱动和应用层之间的“事件出口”。
// 驱动只负责判断发生了什么，不直接打印、不直接控制业务流程；
// 应用层注册函数后，决定要记录日志、亮灯、停机还是上报。
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

  // 数据更新回调（主循环消费CAN反馈事件时触发）
  void (*on_data_update)(C620_HandleTypeDef *hc620);

  // 堵转事件回调
  void (*on_stall)(C620_HandleTypeDef *hc620);

  // 通信恢复回调
  void (*on_comm_recover)(C620_HandleTypeDef *hc620);
} C620_CallbackTypeDef;

typedef struct {
  const char *name; // 指向只读名字，通常来自字符串字面量，放在 Flash/.rodata
                    // 若写成 char *，编译器可能允许你改字符串，运行时会写 Flash 导致异常。
  uint8_t esc_id;
  CAN_HandleTypeDef *hcan;
} C620_ConfigTypeDef;

// ===================== C620电调句柄结构体 =====================
// 句柄(handle)就是“一个设备对象的全部上下文”。
// 裸机嵌入式里不要依赖全局散变量保存状态；把状态收进 handle，
// 后续一个驱动就能管理多个电调，也更容易测试和复用。
struct C620_HandleTypeDef {
  DeviceBase base; // C 语言模拟继承：必须第一个成员，才能把 C620* 当 DeviceBase* 使用
                   // 底层含义：&obj == &obj.base，父类接口收到 DeviceBase* 后可转回子类。
                   // 如果 base 不是第一个成员，这个转换会直接指错地址。

  // 硬件配置
  CAN_HandleTypeDef *hcan; // CAN总线句柄
  uint8_t esc_id;          // 电调ID(1-8)

  // 核心状态
  C620_StateTypeDef state;     // 主状态机
  C620_ErrorTypeDef error;     // 错误状态
  C620_WarningTypeDef warning; // 警告状态

  // 反馈数据（100%对齐官方CAN协议）
  // CAN ISR/接收路径写入这些字段，主循环和应用层读取。
  // 多字段不是原子快照，所以复杂控制算法可再加快照接口；当前测试和状态机已足够。
  // Cortex-M 对对齐的 16/32bit 访问通常是单次总线访问，但“多个字段的一致性”
  // 不是硬件自动保证的：角度、速度、电流可能来自两次不同反馈之间的边界。
  uint16_t mechanical_angle; // 转子机械角度(0~8191对应0~360°)
  uint16_t last_mechanical_angle; // 上一次转子机械角度
  int16_t actual_speed;      // 转子转速(rpm)
  int16_t actual_current;    // 实际转矩电流(-16384~16384)
  uint8_t actual_temp;       // 电机温度(℃)

  // 控制数据
  // target_current 只是“目标值缓存”，真正发到 CAN 总线是在 C620_Send_All_Commands()。
  // 这样多个电调可以被聚合成官方要求的一帧，而不是每个对象各发一帧。
  int16_t target_current; // 目标转矩电流（C620只接收电流指令）
  bool enable_output;     // 输出使能标志
  bool feedback_timeout_enable; // 是否启用反馈超时保护

  // 计时变量
  // 嵌入式安全逻辑通常靠“持续时间”而不是单次采样：
  // 例如堵转/过流必须持续超过阈值，避免因为瞬时噪声误停机。
  uint32_t last_rx_time;     // 最后一次CAN反馈时间
  uint32_t stall_start_time; // 堵转开始时间
  uint32_t over_current_start_time; // 过流开始时间

  // 异步事件标志（CAN接收路径置位，主循环消费）
  // ISR里只做短小确定的事：解析数据、置标志。
  // 真正的日志、状态回调、复杂判断放在主循环，避免中断占用时间过长。
  // volatile 只告诉编译器“每次都从内存读写，不要优化成寄存器缓存”；
  // 它不等于原子操作，也不自动解决竞态，所以清标志仍需要短临界区。
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
 * @brief 电调主循环处理函数（周期调用，用于状态机和事件回调）
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
 * @brief 发送所有已注册C620实例的聚合控制帧
 * @return 操作状态
 */
Device_StatusTypeDef C620_Send_All_Commands(void);

#endif // C620_H
