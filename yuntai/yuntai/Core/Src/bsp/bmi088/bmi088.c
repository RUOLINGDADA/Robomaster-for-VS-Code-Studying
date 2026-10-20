/**
 * @file bmi088.c
 * @brief BMI088 Rev1.9 寄存器、突发 DMA 和物理量转换。
 *
 * 初始化延时由调用者提供；SPI 不能与其他任务并发使用。DMA 缓冲不能放 CCMRAM。
 */
#include "bsp/bmi088/bmi088.h"
#include <string.h>

#define BMI088_READ_BIT 0x80U /* SPI 地址 bit7=1 表示读，手册 §6.1。 */
#define BMI088_RESET_CMD 0xB6U /* 两颗传感器的软复位命令。 */
#define BMI088_GRAVITY_MPS2 9.80665f /* 标准重力加速度，m/s²。 */
#define BMI088_DEG_TO_RAD 0.017453292519943295f /* °/s 转 rad/s。 */

/* 同时释放两颗芯片，防止 SPI 的 MISO 同时被驱动。 */
static void Bmi088_Deselect(Bmi088_HandleTypeDef *handle) {
  HAL_GPIO_WritePin(handle->config.accel_port, handle->config.accel_pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(handle->config.gyro_port, handle->config.gyro_pin, GPIO_PIN_SET);
}

/* 每次事务只拉低目标片选；上一次事务必须已经结束。 */
static void Bmi088_Select(Bmi088_HandleTypeDef *handle, bool accel) {
  Bmi088_Deselect(handle);
  HAL_GPIO_WritePin(accel ? handle->config.accel_port : handle->config.gyro_port,
                    accel ? handle->config.accel_pin : handle->config.gyro_pin, GPIO_PIN_RESET);
}

/* 初始化单寄存器读取；加速度在地址之后还必须发送一个 dummy byte。 */
static bool Bmi088_ReadRegister(Bmi088_HandleTypeDef *handle, bool accel, uint8_t reg, uint8_t *value) {
  uint8_t tx[3] = {(uint8_t)(reg | BMI088_READ_BIT), 0U, 0U};
  uint8_t rx[3] = {0};
  Bmi088_Select(handle, accel);
  HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(handle->config.spi, tx, rx,
                                                     accel ? 3U : 2U, handle->config.timeout_ms);
  Bmi088_Deselect(handle);
  *value = rx[accel ? 2U : 1U];
  return status == HAL_OK;
}

/* 写后至少等待 1 ms，同时满足 suspend 模式 1000 us 的内部同步要求。 */
static bool Bmi088_WriteRegister(Bmi088_HandleTypeDef *handle, bool accel, uint8_t reg, uint8_t value) {
  uint8_t tx[2] = {(uint8_t)(reg & 0x7FU), value};
  Bmi088_Select(handle, accel);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(handle->config.spi, tx, sizeof(tx), handle->config.timeout_ms);
  Bmi088_Deselect(handle);
  handle->config.delay_ms(1U);
  return status == HAL_OK;
}

/* 只比较可写位；GYRO_BANDWIDTH bit7 为只读 1，不能整字节比较。 */
static bool Bmi088_WriteChecked(Bmi088_HandleTypeDef *handle, bool accel,
                                uint8_t reg, uint8_t value, uint8_t mask) {
  uint8_t readback = 0U;
  return Bmi088_WriteRegister(handle, accel, reg, value) &&
      Bmi088_ReadRegister(handle, accel, reg, &readback) && (readback & mask) == (value & mask);
}

/* 用减法解释补码，避免把超出 INT16_MAX 的无符号值直接转换成有符号值。 */
static int16_t Bmi088_DecodeInt16(const uint8_t *bytes) {
  uint32_t value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U);
  return (int16_t)(value >= 32768U ? (int32_t)value - 65536 : (int32_t)value);
}

bool Bmi088_Init(Bmi088_HandleTypeDef *handle, const Bmi088_ConfigTypeDef *config) {
  if (handle == NULL || config == NULL || config->spi == NULL || config->accel_port == NULL ||
      config->gyro_port == NULL || config->accel_pin == 0U || config->gyro_pin == 0U ||
      config->delay_ms == NULL || config->timeout_ms == 0U || config->accel_range > 3U ||
      config->gyro_range > 4U || config->gyro_bandwidth > 7U ||
      (config->accel_conf >> 4U) < 8U || (config->accel_conf >> 4U) > 10U ||
      (config->accel_conf & 15U) < 5U || (config->accel_conf & 15U) > 12U) { return false; }
  memset(handle, 0, sizeof(*handle));
  handle->config = *config;
  Bmi088_Deselect(handle);
  config->delay_ms(10U);
  uint8_t id = 0U;
  /* 首次读只用于产生 CS 上升沿。复位后再次 dummy 读，不把该值作为 ID。 */
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) ||
      !Bmi088_WriteRegister(handle, true, 0x7EU, BMI088_RESET_CMD)) { return false; }
  config->delay_ms(10U);
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) ||
      !Bmi088_WriteRegister(handle, false, 0x14U, BMI088_RESET_CMD)) { return false; }
  config->delay_ms(80U); /* 陀螺仪启动最大 80 ms，手册 §1。 */
  if (!Bmi088_ReadRegister(handle, true, 0x00U, &id) || id != 0x1EU ||
      !Bmi088_ReadRegister(handle, false, 0x00U, &id) || id != 0x0FU) { return false; }
  /* 先开启加速度电源，再写滤波/量程；中断输出低有效，与 CubeMX 下降沿一致。 */
  if (!Bmi088_WriteChecked(handle, true, 0x7DU, 0x04U, 0x04U) ||
      !Bmi088_WriteChecked(handle, true, 0x7CU, 0x00U, 0x03U) ||
      !Bmi088_WriteChecked(handle, true, 0x40U, config->accel_conf, 0xFFU) ||
      !Bmi088_WriteChecked(handle, true, 0x41U, config->accel_range, 0x03U) ||
      !Bmi088_WriteChecked(handle, true, 0x53U, 0x08U, 0x0EU) ||
      !Bmi088_WriteChecked(handle, true, 0x58U, 0x04U, 0x44U) ||
      !Bmi088_WriteChecked(handle, false, 0x0FU, config->gyro_range, 0x07U) ||
      !Bmi088_WriteChecked(handle, false, 0x10U, config->gyro_bandwidth, 0x7FU) ||
      !Bmi088_WriteChecked(handle, false, 0x11U, 0x00U, 0xA0U) ||
      !Bmi088_WriteChecked(handle, false, 0x16U, 0x00U, 0x0FU) ||
      !Bmi088_WriteChecked(handle, false, 0x18U, 0x01U, 0x81U) ||
      !Bmi088_WriteChecked(handle, false, 0x15U, 0x80U, 0x80U)) { return false; }
  config->delay_ms(80U);
  handle->initialized = true;
  return true;
}

bool Bmi088_StartRead(Bmi088_HandleTypeDef *handle, Bmi088_ChannelTypeDef channel) {
  if (handle == NULL || !handle->initialized || handle->dma_state != BMI088_DMA_IDLE ||
      (channel != BMI088_CHANNEL_GYRO && channel != BMI088_CHANNEL_ACCEL && channel != BMI088_CHANNEL_TEMP)) {
    return false;
  }
  memset(handle->tx, 0, sizeof(handle->tx));
  handle->channel = channel;
  uint16_t length = channel == BMI088_CHANNEL_GYRO ? 7U : (channel == BMI088_CHANNEL_ACCEL ? 8U : 4U);
  handle->tx[0] = channel == BMI088_CHANNEL_GYRO ? 0x82U : (channel == BMI088_CHANNEL_ACCEL ? 0x92U : 0xA2U);
  handle->dma_state = BMI088_DMA_BUSY;
  Bmi088_Select(handle, channel != BMI088_CHANNEL_GYRO);
  __DMB(); /* 缓冲和状态先于 DMA 启动可见，完成中断可能立即到达。 */
  if (HAL_SPI_TransmitReceive_DMA(handle->config.spi, handle->tx, handle->rx, length) != HAL_OK) {
    Bmi088_Deselect(handle);
    handle->dma_state = BMI088_DMA_ERROR;
    return false;
  }
  return true;
}

void Bmi088_DmaCompleteFromIsr(Bmi088_HandleTypeDef *handle, bool error) {
  if (handle == NULL || handle->dma_state != BMI088_DMA_BUSY) { return; }
  Bmi088_Deselect(handle);
  __DMB(); /* 先完成 DMA 内存写入，再发布 DONE。 */
  handle->dma_state = error ? BMI088_DMA_ERROR : BMI088_DMA_DONE;
}

bool Bmi088_FinishRead(Bmi088_HandleTypeDef *handle) {
  if (handle == NULL || handle->dma_state != BMI088_DMA_DONE) { return false; }
  __DMB();
  if (handle->channel == BMI088_CHANNEL_TEMP) {
    int32_t raw = ((int32_t)handle->rx[2] << 3U) | (handle->rx[3] >> 5U);
    handle->sample.temperature_valid = raw != 1024; /* 0x80/0x00 为手册无效温度。 */
    handle->sample.temperature_raw = (int16_t)(raw > 1023 ? raw - 2048 : raw);
    handle->sample.temperature_c = 23.0f + handle->sample.temperature_raw * 0.125f;
  } else {
    bool accel = handle->channel == BMI088_CHANNEL_ACCEL;
    float scale = accel ? (3.0f * (float)(1U << handle->config.accel_range) * BMI088_GRAVITY_MPS2 / 32768.0f) :
        (2000.0f / (float)(1U << handle->config.gyro_range) / 32768.0f * BMI088_DEG_TO_RAD);
    for (unsigned axis = 0U; axis < 3U; ++axis) {
      int16_t raw = Bmi088_DecodeInt16(&handle->rx[(accel ? 2U : 1U) + 2U * axis]);
      if (accel) { handle->sample.accel_raw[axis] = raw; handle->sample.accel_mps2[axis] = raw * scale; }
      else { handle->sample.gyro_raw[axis] = raw; handle->sample.gyro_rad_s[axis] = raw * scale; }
    }
  }
  handle->dma_state = BMI088_DMA_IDLE;
  return true;
}

void Bmi088_Abort(Bmi088_HandleTypeDef *handle) {
  if (handle == NULL || handle->config.spi == NULL) { return; }
  handle->initialized = false;
  handle->dma_state = BMI088_DMA_ERROR; /* 后来的完成中断不能再次发布 DONE。 */
  (void)HAL_SPI_Abort(handle->config.spi);
  Bmi088_Deselect(handle);
  handle->dma_state = BMI088_DMA_IDLE;
}
