#ifndef DEVICE_BASE_H
#define DEVICE_BASE_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


// 统一错误码（所有驱动共用）
// 嵌入式驱动不要只返回 true/false：调用者需要区分“参数空”“没初始化”
// “总线忙”“超时”等原因，才能决定是重试、停机还是进入故障状态。
// 这里故意使用带负值的 enum：0 表示 OK，负数表示错误，调试时一眼能分辨。
// 在 ABI 层面 enum 通常按 int 存储，跨模块传参成本低，也方便断点观察。
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
// HAL_GetTick() 是 uint32_t，约 49.7 天会回绕到 0。
// 不能直接写 now > deadline，否则回绕瞬间会判断错。
// 这里把差值转成 int32_t，只要两次比较间隔小于 2^31 ms，就能正确处理回绕。
// 注意：这个技巧利用的是“无符号减法按模 2^32 回绕”的 C 语言规则，
// 再把差值解释成有符号距离；这是裸机时间轮常用写法。
#define is_time_A_before_B(a, b) ((int32_t)((a) - (b)) < 0)
#define is_time_A_after_B(a, b) ((int32_t)((a) - (b)) > 0)
#define is_time_A_before_B_eq(a, b) ((int32_t)((a) - (b)) <= 0)
#define is_time_A_after_B_eq(a, b) ((int32_t)((a) - (b)) >= 0)

// 运行时断言（调试用，Release版可定义NDEBUG关闭）
// 在 MCU 上没有操作系统帮你弹异常窗口；调试期直接卡死在这里，
// 配合断点/调用栈能快速定位是哪一次参数传错。
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

// 设备基类：用 C 模拟“父类”。
// 父类只放所有设备都共有的能力：名字、ID、初始化标志、析构函数。
// 子类例如 C620_HandleTypeDef 把 DeviceBase 放在结构体第一个成员，
// 这样“子类指针地址 == 第一个成员地址”，就能安全地把子类当父类看。
// 这是结构体布局规则带来的能力：第一个成员偏移为 0。
// 工业代码里常用 container_of/首成员继承等技巧，但前提是必须严格遵守布局约定。
typedef struct DeviceBase {
  const char *name;    // 常量指针读法：从右往左看，name 是指针，指向 const char
                       // 指针变量本身可以改指向，但指向的字符串内容不能改。
                       // 字符串字面量通常放在 Flash/.rodata，不占用宝贵 RAM。
  uint8_t device_id;   // 设备ID
  bool is_initialized; // 初始化完成标志
  // 设备析构函数。参数写 struct DeviceBase *，因为在 typedef 右花括号之前，
  // 别名 DeviceBase 还没有正式生效；struct DeviceBase 是“结构体标签名”。
  // 这里是函数指针，不是函数调用；它把“如何销毁自己”这个行为放进对象里，
  // 这就是 C 语言实现多态/虚函数表的最小雏形。
  Device_StatusTypeDef (*deinit)(struct DeviceBase *dev);
} DeviceBase;

// 统一命令表结构（所有驱动共用）
// 这是一种嵌入式常见“命令分发”写法：上层只给 cmd_id，
// 驱动内部查表找到 handler，避免写一长串 if/else。
typedef struct {
  uint8_t cmd_id;                                        // 命令ID
  Device_StatusTypeDef (*handler)(void *dev, void *arg); // 函数指针：把不同命令绑定到不同处理函数
                                                         // void * 是“类型擦除”：命令表不知道具体设备类型，
                                                         // handler 内部再转回自己的 C620_HandleTypeDef。
  uint16_t arg_size;                                     // 参数大小（字节）
  const char *cmd_name;                                  // 命令名（调试用）
} Command_TableTypeDef;

#endif // DEVICE_BASE_H
