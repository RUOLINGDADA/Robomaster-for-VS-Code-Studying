/**
 * @file ist8310.h
 * @brief IST8310 I2C 驱动；只在独占 I2C 的任务中初始化和读取。
 * 磁场标定、安装旋转与融合由上层完成；这里不创建任务、不打印。
 */
#ifndef BSP_IST8310_H
#define BSP_IST8310_H /* 防止重复包含。 */
#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef enum {
  IST8310_READ_ERROR, /* 总线、参数或初始化错误。 */
  IST8310_READ_NOT_READY, /* 没有新数据，不能刷新样本时间。 */
  IST8310_READ_OK, /* 一个新的有效样本。 */
  IST8310_READ_OVERRUN /* 旧样本被覆盖；本次最新样本仍可使用并计数。 */
} Ist8310_ReadResultTypeDef;

typedef enum {
  IST8310_INIT_RESET_LOW, /* 等待低电平复位最小保持时间。 */
  IST8310_INIT_RESET_HIGH, /* 释放复位后等待器件启动。 */
  IST8310_INIT_WRITE, /* 下一个配置写事务。 */
  IST8310_INIT_VERIFY, /* 写后等待，再执行回读事务。 */
  IST8310_INIT_READY, /* 可以采样。 */
  IST8310_INIT_ERROR /* 等待任务重新 BeginInit。 */
} Ist8310_InitStateTypeDef;

typedef struct {
  I2C_HandleTypeDef *i2c; /* 所有者任务独占。 */
  GPIO_TypeDef *reset_port; /* RSTN 端口。 */
  uint16_t reset_pin; /* RSTN，低有效。 */
  void (*delay_ms)(uint32_t ms); /* 初始化任务延时。 */
  uint32_t timeout_ms; /* 每次 HAL 事务超时，ms，非零。 */
} Ist8310_ConfigTypeDef;

typedef struct {
  Ist8310_ConfigTypeDef config; /* 初始化硬件配置。 */
  int16_t raw[3]; /* 芯片坐标系，LSB。 */
  float mag_ut[3]; /* 芯片坐标系，μT；尚未标定。 */
  uint8_t status; /* STAT1 原始值，bit0 新数据、bit1 覆盖。 */
  bool initialized; /* 设备 ID 和配置全部通过回读。 */
  Ist8310_InitStateTypeDef init_state; /* 任务驱动的初始化阶段。 */
  uint32_t phase_ms; /* 阶段起点，HAL ms。 */
  uint8_t register_index; /* 四项配置中的当前位置，0~3。 */
} Ist8310_HandleTypeDef;

/** @brief 复位、验证 ID 并设 200 Hz。@param handle 所有者句柄。@param config 硬件配置。
 * @return 成功为 true；失败不阻塞其他任务，不允许读取。仅任务调用。 */
bool Ist8310_Init(Ist8310_HandleTypeDef *handle, const Ist8310_ConfigTypeDef *config);
/** @brief 开始非阻塞复位。@param handle 长期句柄。@param config 硬件参数。@param now_ms HAL 时间。
 * @return 参数有效为 true；仅任务调用，用于不暂停 1kHz 融合的磁力计恢复。 */
bool Ist8310_BeginInit(Ist8310_HandleTypeDef *handle, const Ist8310_ConfigTypeDef *config, uint32_t now_ms);
/** @brief 推进最多一个 I2C 事务。@param handle 长期句柄。@param now_ms HAL 时间。
 * @return 当前初始化状态；仅任务调用，等待阶段立即返回。 */
Ist8310_InitStateTypeDef Ist8310_PollInit(Ist8310_HandleTypeDef *handle, uint32_t now_ms);
/** @brief 读取 STAT1 与六字节磁场。@param handle 所有者句柄。
 * @return 新样本/覆盖/未就绪/错误；仅新样本或覆盖更新 raw 和 mag_ut。仅任务调用。 */
Ist8310_ReadResultTypeDef Ist8310_Read(Ist8310_HandleTypeDef *handle);
#endif
