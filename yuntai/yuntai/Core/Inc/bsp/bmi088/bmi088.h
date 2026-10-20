/**
 * @file bmi088.h
 * @brief BMI088 SPI 双片选驱动；初始化在任务中，采集使用持久 DMA 缓冲。
 *
 * 只管理寄存器、单位和传输状态。调用者独占 SPI，不在 ISR 解析数据或等待。
 */
#ifndef BSP_BMI088_H
#define BSP_BMI088_H /* 防止重复包含。 */

#include "stm32f4xx_hal.h"
#include <stdbool.h>

typedef enum {
  BMI088_CHANNEL_GYRO, /* 六字节角速度。 */
  BMI088_CHANNEL_ACCEL, /* 六字节加速度。 */
  BMI088_CHANNEL_TEMP /* 两字节温度。 */
} Bmi088_ChannelTypeDef;

typedef enum {
  BMI088_DMA_IDLE, /* 缓冲可以重新使用。 */
  BMI088_DMA_BUSY, /* DMA 拥有缓冲。 */
  BMI088_DMA_DONE, /* ISR 已释放片选，任务可以解析。 */
  BMI088_DMA_ERROR /* 任务必须中止并重新初始化。 */
} Bmi088_DmaStateTypeDef;

typedef struct {
  SPI_HandleTypeDef *spi; /* 任务独占的 SPI。 */
  GPIO_TypeDef *accel_port; /* 加速度片选端口。 */
  uint16_t accel_pin; /* 低有效片选。 */
  GPIO_TypeDef *gyro_port; /* 陀螺仪片选端口。 */
  uint16_t gyro_pin; /* 低有效片选。 */
  void (*delay_ms)(uint32_t ms); /* 任务延时；不得在 ISR 初始化。 */
  uint32_t timeout_ms; /* 初始化 HAL 事务超时，ms，必须非零。 */
  uint8_t accel_range; /* ACC_RANGE：0~3 对应 ±3/6/12/24 g。 */
  uint8_t accel_conf; /* ACC_CONF：带宽和 ODR，见 BMI088 Rev1.9 §5.3.10。 */
  uint8_t gyro_range; /* GYRO_RANGE：0~4 对应 ±2000/1000/500/250/125 °/s。 */
  uint8_t gyro_bandwidth; /* GYRO_BANDWIDTH：0~7，2 为 1000 Hz/116 Hz。 */
} Bmi088_ConfigTypeDef;

typedef struct {
  int16_t accel_raw[3]; /* 芯片坐标系，LSB。 */
  int16_t gyro_raw[3]; /* 芯片坐标系，LSB。 */
  int16_t temperature_raw; /* 11 位符号扩展后的温度，LSB。 */
  float accel_mps2[3]; /* 芯片坐标系比力，m/s²。 */
  float gyro_rad_s[3]; /* 芯片坐标系角速度，rad/s。 */
  float temperature_c; /* 摄氏度；有效性由 temperature_valid 表示。 */
  bool temperature_valid; /* 首次成功读取且不为手册无效码时置位。 */
} Bmi088_SampleTypeDef;

typedef struct {
  Bmi088_ConfigTypeDef config; /* 初始化后只由所有者任务修改。 */
  Bmi088_SampleTypeDef sample; /* 仅任务在 DMA 完成后更新。 */
  uint8_t tx[8]; /* 主 RAM 持久缓冲；DMA 完成前不能改写。 */
  uint8_t rx[8]; /* 主 RAM 持久缓冲；ISR 完成后才可读取。 */
  Bmi088_ChannelTypeDef channel; /* 当前传输，BUSY 期间保持不变。 */
  volatile Bmi088_DmaStateTypeDef dma_state; /* ISR 发布状态；不代表多字段快照。 */
  bool initialized; /* ID、配置写入及回读全部成功。 */
} Bmi088_HandleTypeDef;

/** @brief 初始化并检查配置。@param handle 长期有效句柄。@param config 硬件及寄存器配置。
 * @return 成功为 true；失败保持未初始化、两个片选为高。仅任务调用。 */
bool Bmi088_Init(Bmi088_HandleTypeDef *handle, const Bmi088_ConfigTypeDef *config);
/** @brief 启动一次突发 DMA。@param handle 已初始化句柄。@param channel 数据类型。
 * @return true 表示 DMA 接受；false 不产生新样本。仅所有者任务调用。 */
bool Bmi088_StartRead(Bmi088_HandleTypeDef *handle, Bmi088_ChannelTypeDef channel);
/** @brief 解析完成的传输。@param handle 所有者句柄。
 * @return 成功为 true；BUSY/ERROR 不解析。更新 sample 后释放缓冲。仅任务调用。 */
bool Bmi088_FinishRead(Bmi088_HandleTypeDef *handle);
/** @brief 结束 DMA 并释放片选。@param handle 所有者句柄。@return 无。
 * @note 仅任务调用；中止后必须重新初始化，不复用可能残留的完成事件。 */
void Bmi088_Abort(Bmi088_HandleTypeDef *handle);
/** @brief HAL 完成/错误回调适配。@param handle 长期句柄。@param error 是否传输错误。
 * @return 无；仅 ISR 调用，不做浮点运算，调用者负责通知任务。 */
void Bmi088_DmaCompleteFromIsr(Bmi088_HandleTypeDef *handle, bool error);

#endif
