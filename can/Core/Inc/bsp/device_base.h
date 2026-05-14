#ifndef __DEVICE_H
#define __DEVICE_H
#include <stdbool.h>

#define DEBUG 1
// 定义所有设备通用状态 错误码用负数表示
typedef enum {
  DEVICE_OK = 0,
  DEVICE_ERROR = -1,
  DEVICE_NO_PARAMETER = -2,
  DEVICE_NOT_INITIALIZED = -3,
  DEVICE_TIMEOUT = -4,
  DEVICE_INVALID_PARAM = -5,
  DEVICE_BUSY = -6
} Device_StatusTypeDef;

// 设备参数断言
#ifdef DEBUG
#define device_assert_param(expr)                                              \
  do {                                                                         \
    if (!(expr)) {                                                             \
      while (1)                                                                \
        ;                                                                      \
    }                                                                          \
  } while (0)
#endif

// 定义所有设备基类:父类只定义公共能力，子类扩展私有特性
typedef struct DeviceBase {
  const char *
      device_name; // 设备名
                   // 这是常量指针:指针可以随便指，但指向的东西不能改(从右往左看)
  uint8_t device_id;   // 设备id
  bool is_initialized; // 设备初始化标记
  Device_StatusTypeDef (*deinit)(
      struct DeviceBase *dev); // 设备析构函数 析构的是DeviceBase 这里参数用的是
                               // struct DeviceBase(标签名) 是因为别名
                               // DeviceBaseTypeDef还没生效
} DeviceBaseTypeDef;

// 定义所有设备命令表结构
typedef struct {
  uint8_t id;        // 命令id(可对应设备枚举)
  const char *name;  // 命令名称(常量指针)
  uint16_t arg_size; // 命令参数大小(根据实际情况设计)
  Device_StatusTypeDef (*handler)(void *dev, void *arg); // 命令执行函数
} Command_TableTypeDef;
#endif