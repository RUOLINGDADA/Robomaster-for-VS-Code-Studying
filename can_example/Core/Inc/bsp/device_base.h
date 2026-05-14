#ifndef DEVICE_BASE_H
#define DEVICE_BASE_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


// 统一错误码（所有驱动共用）
typedef enum {
  DEVICE_OK = 0,
  DEVICE_ERROR = -1,
  DEVICE_NO_PARAMETER = -2,
  DEVICE_NOT_INITIALIZED = -3,
  DEVICE_TIMEOUT = -4,
  DEVICE_INVALID_PARAM = -5,
  DEVICE_BUSY = -6
} Device_StatusTypeDef;

// 统一时间比较宏（无溢出，所有驱动共用）
#define is_time_A_before_B(a, b) ((int32_t)((a) - (b)) < 0)
#define is_time_A_after_B(a, b) ((int32_t)((a) - (b)) > 0)
#define is_time_A_before_B_eq(a, b) ((int32_t)((a) - (b)) <= 0)
#define is_time_A_after_B_eq(a, b) ((int32_t)((a) - (b)) >= 0)

// 运行时断言（调试用，Release版可定义NDEBUG关闭）
#ifndef NDEBUG
#define DEVICE_ASSERT(expr)                                                    \
  do {                                                                         \
    if (!(expr)) {                                                             \
      while (1)                                                                \
        ; /* 调试时直接卡死，快速定位空指针 */                                 \
    }                                                                          \
  } while (0)
#else
#define DEVICE_ASSERT(expr) ((void)0)
#endif

// 设备基类（所有驱动必须第一个成员继承）
typedef struct DeviceBase {
  const char *name;    // 设备名（调试日志用）
  uint8_t device_id;   // 设备ID
  bool is_initialized; // 初始化完成标志
  Device_StatusTypeDef (*deinit)(struct DeviceBase *dev); // 析构函数
} DeviceBase;

// 统一命令表结构（所有驱动共用）
typedef struct {
  uint8_t cmd_id;                                        // 命令ID
  Device_StatusTypeDef (*handler)(void *dev, void *arg); // 命令处理函数
  uint16_t arg_size;                                     // 参数大小（字节）
  const char *cmd_name;                                  // 命令名（调试用）
} Command_TableTypeDef;

#endif // DEVICE_BASE_H