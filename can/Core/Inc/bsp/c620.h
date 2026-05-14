/*
设备状态,设备出错对应的错误码,构造函数,析构函数,回调函数(包含错误回调,空回调函数),
可选:(平台选择->对应底层处理函数),(命令执行函数)
*/

#ifndef __C620_H
#define __C620_H
#include "can_ex.h"
#include "device_base.h"
#include "main.h"
#include "stm32f4xx_hal_can.h"
#include <stdlib.h>

#define IS_C620_ID(INSTANCE)                                                   \
  (((INSTANCE) == 1) || ((INSTANCE) == 2) || ((INSTANCE) == 3) ||              \
   ((INSTANCE) == 4) || ((INSTANCE) == 5) || ((INSTANCE) == 6) ||              \
   ((INSTANCE) == 7) || ((INSTANCE) == 8))

// C620状态
typedef enum { C620_OK = 0, C620_ERROR = -1 } C620_StatusTypeDef;
// C620控制的电机状态
typedef enum { MOTOR_OK = 0, MOTOR_ERROR = -1 } Motor_StatusTypeDef;
// C620错误码
typedef enum {
  C620_ERROR_NONE = 0,             // 无异常
  C620_ERROR_MOTOR_EEPROM = 1,     // 无法访问电机存储芯片（开机自检）
  C620_ERROR_OVER_VOLTAGE = 2,     // 供电电压过高（开机自检）
  C620_ERROR_PHASE_MISSING = 3,    // 电机三相线未接入
  C620_ERROR_ENCODER_LOST = 4,     // 位置传感器数据丢失
  C620_ERROR_OVER_TEMP_CRIT = 5,   // 电机温度≥180℃（致命错误）
  C620_ERROR_CALIBRATION_FAIL = 7, // 电机校准失败
  C620_ERROR_OVER_TEMP_WARN = 8    // 电机温度≥125℃（警告）
} C620_ErrorCodeTypeDef;
// C620命令
typedef enum { MOTOR_START = 0, MOTOR_STOP } C620_CMD_TypeDef;
// 声明C620不透明结构体
typedef struct C620_HandleTypeDef C620_HandleTypeDef;
// 回调函数表
typedef struct {
  // 错误回调
  void (*on_error)(C620_HandleTypeDef *hc620, C620_ErrorCodeTypeDef error);
  // 数据更新回调
  void (*on_data_update)(C620_HandleTypeDef *hc620);
} C620_CallBackTypeDef;
// 构造函数
C620_HandleTypeDef *C620_Create(const char *name, CAN_HandleTypeDef *hcan,
                                uint8_t c620_id);

// 析构函数
C620_StatusTypeDef C620_Destroy(C620_HandleTypeDef *hc620);
// 对外接口

#endif