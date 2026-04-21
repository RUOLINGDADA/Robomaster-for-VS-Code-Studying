#include "ist8130.h"
#include <string.h>

// ********** 私有函数声明 **********
static HAL_StatusTypeDef IST8130_WriteReg(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t data);
static HAL_StatusTypeDef IST8130_ReadReg(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t *data);
static HAL_StatusTypeDef IST8130_ReadRegs(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t *data, uint8_t len);

/**
  * @brief  IST8130初始化函数
  * @param  handle: IST8130句柄
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态
  */
HAL_StatusTypeDef IST8130_Init(IST8130_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t id;

    // 初始化句柄
    memset(handle, 0, sizeof(IST8130_HandleTypeDef));
    handle->hi2c = hi2c;
    handle->status = 0;
    handle->is_init = 0;

    // 复位传感器（可选）
    ret = IST8130_WriteReg(handle, IST8130_CNTL2, 0x01);
    if (ret != HAL_OK) return ret;
    HAL_Delay(10);

    // 读取设备ID
    ret = IST8130_ReadID(handle, &id);
    if (ret != HAL_OK || id != IST8130_WHO_AM_I_VAL)
    {
        return HAL_ERROR;
    }

    // 配置控制寄存器
    ret = IST8130_WriteReg(handle, IST8130_CNTL3, 0x00);  // 禁用中断
    if (ret != HAL_OK) return ret;

    // 设置为连续测量模式（100Hz）
    ret = IST8130_SetMode(handle, IST8130_MODE_CONTINUOUS_100HZ);
    if (ret != HAL_OK) return ret;

    handle->is_init = 1;
    return HAL_OK;
}

/**
  * @brief  读取设备ID
  * @param  handle: IST8130句柄
  * @param  id: 存储ID的指针
  * @retval HAL状态
  */
HAL_StatusTypeDef IST8130_ReadID(IST8130_HandleTypeDef *handle, uint8_t *id)
{
    return IST8130_ReadReg(handle, IST8130_WHO_AM_I, id);
}

/**
  * @brief  设置工作模式
  * @param  handle: IST8130句柄
  * @param  mode: 工作模式
  * @retval HAL状态
  */
HAL_StatusTypeDef IST8130_SetMode(IST8130_HandleTypeDef *handle, uint8_t mode)
{
    return IST8130_WriteReg(handle, IST8130_CNTL1, mode);
}

/**
  * @brief  读取磁场数据
  * @param  handle: IST8130句柄
  * @retval HAL状态
  */
HAL_StatusTypeDef IST8130_ReadData(IST8130_HandleTypeDef *handle)
{
    if (!handle->is_init) return HAL_ERROR;

    uint8_t data[6];
    HAL_StatusTypeDef ret = IST8130_ReadRegs(handle, IST8130_DATA_X_L, data, 6);
    if (ret != HAL_OK) return ret;

    // 组合数据（低字节在前，高字节在后）
    handle->mag_x = (int16_t)((data[1] << 8) | data[0]);
    handle->mag_y = (int16_t)((data[3] << 8) | data[2]);
    handle->mag_z = (int16_t)((data[5] << 8) | data[4]);

    // 清除硬件DRDY标志（读取数据后自动清除）
    IST8130_ClearDataReadyFlag(handle);

    return HAL_OK;
}

/**
  * @brief  检查数据就绪状态
  * @param  handle: IST8130句柄
  * @param  is_ready: 就绪状态
  * @retval HAL状态
  */
HAL_StatusTypeDef IST8130_CheckDRDY(IST8130_HandleTypeDef *handle, uint8_t *is_ready)
{
    uint8_t stat1;
    HAL_StatusTypeDef ret = IST8130_ReadReg(handle, IST8130_STAT1, &stat1);
    if (ret != HAL_OK) return ret;

    *is_ready = (stat1 & (1 << IST8130_DATA_READY_BIT)) ? 1 : 0;
    if (*is_ready)
    {
        IST8130_SetDataReadyFlag(handle);
    }

    return HAL_OK;
}

/**
  * @brief  设置数据就绪标志位
  * @param  handle: IST8130句柄
  */
void IST8130_SetDataReadyFlag(IST8130_HandleTypeDef *handle)
{
    handle->status |= 1 << IST8130_DATA_READY_BIT;
}

/**
  * @brief  清除数据就绪标志位
  * @param  handle: IST8130句柄
  */
void IST8130_ClearDataReadyFlag(IST8130_HandleTypeDef *handle)
{
    handle->status &= ~(1 << IST8130_DATA_READY_BIT);
}

/**
  * @brief  检查数据就绪标志位
  * @param  handle: IST8130句柄
  * @retval 就绪状态
  */
uint8_t IST8130_IsDataReady(IST8130_HandleTypeDef *handle)
{
    return (handle->status & (1 << IST8130_DATA_READY_BIT)) ? 1 : 0;
}

// ********** 私有函数实现 **********

/**
  * @brief  写入单个寄存器
  * @param  handle: IST8130句柄
  * @param  reg: 寄存器地址
  * @param  data: 写入数据
  * @retval HAL状态
  */
static HAL_StatusTypeDef IST8130_WriteReg(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t data)
{
    uint8_t tx_data[2] = {reg, data};
    return HAL_I2C_Master_Transmit(handle->hi2c, IST8130_I2C_ADDR, tx_data, 2, 100);
}

/**
  * @brief  读取单个寄存器
  * @param  handle: IST8130句柄
  * @param  reg: 寄存器地址
  * @param  data: 读取数据
  * @retval HAL状态
  */
static HAL_StatusTypeDef IST8130_ReadReg(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t *data)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(handle->hi2c, IST8130_I2C_ADDR, &reg, 1, 100);
    if (ret != HAL_OK) return ret;
    return HAL_I2C_Master_Receive(handle->hi2c, IST8130_I2C_ADDR, data, 1, 100);
}

/**
  * @brief  读取多个寄存器
  * @param  handle: IST8130句柄
  * @param  reg: 起始寄存器地址
  * @param  data: 存储数据的缓冲区
  * @param  len: 读取长度
  * @retval HAL状态
  */
static HAL_StatusTypeDef IST8130_ReadRegs(IST8130_HandleTypeDef *handle, uint8_t reg, uint8_t *data, uint8_t len)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(handle->hi2c, IST8130_I2C_ADDR, &reg, 1, 100);
    if (ret != HAL_OK) return ret;
    return HAL_I2C_Master_Receive(handle->hi2c, IST8130_I2C_ADDR, data, len, 100);
}