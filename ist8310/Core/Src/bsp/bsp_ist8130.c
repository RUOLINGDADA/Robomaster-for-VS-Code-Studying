#include "bsp_ist8130.h"

static HAL_StatusTypeDef BSP_IST8130_WriteReg(IST8130_HandleTypeDef *handle,
                                              uint16_t MemAddress,
                                              uint8_t *pData, uint16_t Size) {

  return HAL_I2C_Mem_Write(handle->hi2c, IST8130_DEV_ADDR, MemAddress,
                           I2C_MEMADD_SIZE_8BIT, pData, Size, 100);
}

static HAL_StatusTypeDef BSP_IST8130_ReadReg(IST8130_HandleTypeDef *handle,
                                             uint16_t MemAddress,
                                             uint8_t *pData, uint16_t Size) {
  return HAL_I2C_Mem_Read(handle->hi2c, IST8130_DEV_ADDR, MemAddress,
                          I2C_MEMADD_SIZE_8BIT, pData, Size, 100);
}