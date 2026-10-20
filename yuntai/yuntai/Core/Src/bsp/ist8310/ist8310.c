/**
 * @file ist8310.c
 * @brief 依据 C 型板教程 §11 与官方例程配置 IST8310。
 * 任务中使用有界 I2C 事务；不在 EXTI 回调中读取。bit1 只表示覆盖，不是磁饱和。
 */
#include "bsp/ist8310/ist8310.h"
#include <string.h>

#define IST8310_HAL_ADDRESS (0x0EU << 1U) /* 七位地址 0x0E，HAL 要求左移一位。 */
#define IST8310_UT_PER_LSB 0.3f /* 教程 §11.4.3，μT/LSB。 */

/* 所有寄存器地址均为 8 位；HAL 负责重复起始和 ACK。 */
static bool Ist8310_ReadRegisters(Ist8310_HandleTypeDef *handle, uint8_t reg, uint8_t *bytes, uint16_t count) {
  return HAL_I2C_Mem_Read(handle->config.i2c, IST8310_HAL_ADDRESS, reg, I2C_MEMADD_SIZE_8BIT,
                          bytes, count, handle->config.timeout_ms) == HAL_OK;
}

bool Ist8310_BeginInit(Ist8310_HandleTypeDef *handle, const Ist8310_ConfigTypeDef *config, uint32_t now_ms) {
  if (handle == NULL || config == NULL || config->i2c == NULL || config->reset_port == NULL ||
      config->reset_pin == 0U || config->delay_ms == NULL || config->timeout_ms == 0U) { return false; }
  memset(handle, 0, sizeof(*handle));
  handle->config = *config;
  HAL_GPIO_WritePin(config->reset_port, config->reset_pin, GPIO_PIN_RESET);
  handle->init_state=IST8310_INIT_RESET_LOW;
  handle->phase_ms=now_ms;
  return true;
}

Ist8310_InitStateTypeDef Ist8310_PollInit(Ist8310_HandleTypeDef *h, uint32_t now_ms) {
  if (h==NULL || h->config.i2c==NULL) { return IST8310_INIT_ERROR; }
  static const uint8_t registers[4][2]={{0x0BU,0x08U},{0x41U,0x09U},{0x42U,0xC0U},{0x0AU,0x0BU}};
  uint8_t value=0U;
  if (h->init_state==IST8310_INIT_RESET_LOW && (uint32_t)(now_ms-h->phase_ms)>=50U) {
    HAL_GPIO_WritePin(h->config.reset_port,h->config.reset_pin,GPIO_PIN_SET);
    h->phase_ms=now_ms; h->init_state=IST8310_INIT_RESET_HIGH;
  } else if (h->init_state==IST8310_INIT_RESET_HIGH && (uint32_t)(now_ms-h->phase_ms)>=50U) {
    h->init_state=Ist8310_ReadRegisters(h,0x00U,&value,1U) && value==0x10U ? IST8310_INIT_WRITE : IST8310_INIT_ERROR;
  } else if (h->init_state==IST8310_INIT_WRITE) {
    value=registers[h->register_index][1];
    h->init_state=HAL_I2C_Mem_Write(h->config.i2c,IST8310_HAL_ADDRESS,registers[h->register_index][0],
        I2C_MEMADD_SIZE_8BIT,&value,1U,h->config.timeout_ms)==HAL_OK ? IST8310_INIT_VERIFY : IST8310_INIT_ERROR;
    h->phase_ms=now_ms;
  } else if (h->init_state==IST8310_INIT_VERIFY && (uint32_t)(now_ms-h->phase_ms)>=1U) {
    if (!Ist8310_ReadRegisters(h,registers[h->register_index][0],&value,1U) || value!=registers[h->register_index][1]) {
      h->init_state=IST8310_INIT_ERROR;
    } else if (++h->register_index==4U) { h->init_state=IST8310_INIT_READY; h->initialized=true; }
    else { h->init_state=IST8310_INIT_WRITE; }
  }
  return h->init_state;
}

bool Ist8310_Init(Ist8310_HandleTypeDef *handle, const Ist8310_ConfigTypeDef *config) {
  if (!Ist8310_BeginInit(handle,config,HAL_GetTick())) { return false; }
  /* 便捷阻塞接口只用于启动；1kHz 运行中的恢复必须使用 PollInit，不能暂停融合100ms。 */
  while (Ist8310_PollInit(handle,HAL_GetTick())!=IST8310_INIT_READY) {
    if (handle->init_state==IST8310_INIT_ERROR) { return false; }
    config->delay_ms(1U);
  }
  return true;
}

Ist8310_ReadResultTypeDef Ist8310_Read(Ist8310_HandleTypeDef *handle) {
  if (handle == NULL || !handle->initialized) { return IST8310_READ_ERROR; }
  uint8_t bytes[7];
  if (!Ist8310_ReadRegisters(handle, 0x02U, bytes, sizeof(bytes))) { return IST8310_READ_ERROR; }
  handle->status = bytes[0];
  if ((bytes[0] & 0x01U) == 0U) { return IST8310_READ_NOT_READY; }
  for (unsigned axis = 0U; axis < 3U; ++axis) {
    uint32_t raw = (uint32_t)bytes[1U + axis * 2U] | ((uint32_t)bytes[2U + axis * 2U] << 8U);
    handle->raw[axis] = (int16_t)(raw >= 32768U ? (int32_t)raw - 65536 : (int32_t)raw);
    handle->mag_ut[axis] = handle->raw[axis] * IST8310_UT_PER_LSB;
  }
  return (bytes[0] & 0x02U) != 0U ? IST8310_READ_OVERRUN : IST8310_READ_OK;
}
