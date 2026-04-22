#include "bsp_ist8310.h"

static HAL_StatusTypeDef _BSP_IST8310_WriteReg(IST8310_HandleTypeDef *handle,
                                               uint16_t MemAddress,
                                               uint8_t *pData, uint16_t Size) {

  return HAL_I2C_Mem_Write(handle->hi2c, IST8310_DEV_ADDR, MemAddress,
                           I2C_MEMADD_SIZE_8BIT, pData, Size, I2C_TIMEOUT_MAX);
}

static HAL_StatusTypeDef _BSP_IST8310_ReadReg(IST8310_HandleTypeDef *handle,
                                              uint16_t MemAddress,
                                              uint8_t *pData, uint16_t Size) {
  return HAL_I2C_Mem_Read(handle->hi2c, IST8310_DEV_ADDR, MemAddress,
                          I2C_MEMADD_SIZE_8BIT, pData, Size, I2C_TIMEOUT_MAX);
}

HAL_StatusTypeDef BSP_IST8310_SetMode(IST8310_HandleTypeDef *handle,
                                      uint8_t mode) {
  if (handle->is_init != 1) {
    return HAL_ERROR;
  }
  uint8_t reg_val = 0;
  if (_BSP_IST8310_ReadReg(handle, IST8310_CNTL1, &reg_val, 1) != HAL_OK) {
    return HAL_ERROR; // 读取寄存器失败
  }
  reg_val &= 0xF0;          // 清除低4位
  reg_val |= (mode & 0x0F); // 设置新模式
  if (_BSP_IST8310_WriteReg(handle, IST8310_CNTL1, &reg_val, sizeof(reg_val)) !=
      HAL_OK) {
    return HAL_ERROR; // 写入模式失败
  }
  return HAL_OK;
}

HAL_StatusTypeDef BSP_IST8310_Init(IST8310_HandleTypeDef *handle,
                                   I2C_HandleTypeDef *hi2c) {
  handle->hi2c = hi2c;
  uint8_t who_am_i = 0;
  if (_BSP_IST8310_ReadReg(handle, IST8310_WHO_AM_I, &who_am_i, 1) != HAL_OK) {
    return HAL_ERROR; // 读取设备ID失败
  }
  if (who_am_i != IST8130_WHO_AM_I_VAL) {
    return HAL_ERROR; // 设备ID不匹配
  }
  handle->is_init = 1;
  return BSP_IST8310_SetMode(handle, IST8310_MODE_CONTINUOUS);
}

HAL_StatusTypeDef BSP_IST8310_GetStatus(IST8310_HandleTypeDef *handle,
                                        uint8_t *status) {
  if (handle->is_init != 1) {
    return HAL_ERROR;
  }
  if (_BSP_IST8310_ReadReg(handle, IST8310_STAT1, status, 1) != HAL_OK) {
    return HAL_ERROR; // 读取状态寄存器失败
  }
  return HAL_OK;
}

HAL_StatusTypeDef BSP_IST8310_ReadData(IST8310_HandleTypeDef *handle) {
  if (handle->is_init != 1) {
    return HAL_ERROR;
  }

  uint8_t status;
  if (BSP_IST8310_GetStatus(handle, &status) != HAL_OK) {
    return HAL_ERROR; // 获取状态失败
  }

  if (status & (1 << IST8310_DATA_READY_BIT_POS)) {
    uint8_t mag[6];
    HAL_StatusTypeDef ret =
        _BSP_IST8310_ReadReg(handle, IST8310_DATA_X_L, mag, 6);
    if (ret != HAL_OK) {
      return HAL_ERROR;
    }

    handle->mag_x = (int16_t)(mag[1] << 8 | mag[0]);
    handle->mag_y = (int16_t)(mag[3] << 8 | mag[2]);
    handle->mag_z = (int16_t)(mag[5] << 8 | mag[4]);
    return HAL_OK;
  }

  return HAL_BUSY;
}