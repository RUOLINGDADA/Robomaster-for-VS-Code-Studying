#ifndef IST8130_H
#define IST8130_H

#include "stm32f4xx_hal.h"  // 根据你的STM32型号修改

// ********** 设备参数定义 **********
#define IST8130_I2C_ADDR          0x0E<<1  // 7位地址0x0E，左移1位含读写位
#define IST8130_WHO_AM_I          0x00     // 设备ID寄存器地址
#define IST8130_WHO_AM_I_VAL      0x10     // 设备ID值（IST8130特有）

// ********** 寄存器地址定义 **********
#define IST8130_STAT1             0x02     // 状态寄存器1（DRDY标志）
#define IST8130_DATA_X_L          0x03     // X轴数据低字节
#define IST8130_DATA_X_H          0x04     // X轴数据高字节
#define IST8130_DATA_Y_L          0x05     // Y轴数据低字节
#define IST8130_DATA_Y_H          0x06     // Y轴数据高字节
#define IST8130_DATA_Z_L          0x07     // Z轴数据低字节
#define IST8130_DATA_Z_H          0x08     // Z轴数据高字节
#define IST8130_CNTL1             0x0A     // 控制寄存器1
#define IST8130_CNTL2             0x0B     // 控制寄存器2
#define IST8130_CNTL3             0x0C     // 控制寄存器3

// ********** 状态位定义 **********
#define IST8130_DATA_READY_BIT    0        // DRDY标志位于STAT1寄存器bit0

// ********** 工作模式定义 **********
#define IST8130_MODE_SINGLE       0x01     // 单次测量模式
#define IST8130_MODE_CONTINUOUS   0x02     // 连续测量模式（10Hz）
#define IST8130_MODE_CONTINUOUS_100HZ 0x06 // 连续测量模式（100Hz）
#define IST8130_MODE_POWERDOWN    0x00     // 掉电模式

// ********** 数据结构体 **********
typedef struct
{
    int16_t mag_x;         // X轴磁场数据（单位：uT）
    int16_t mag_y;         // Y轴磁场数据（单位：uT）
    int16_t mag_z;         // Z轴磁场数据（单位：uT）
    uint8_t status;        // 状态标志（bit0:数据就绪）
    I2C_HandleTypeDef *hi2c;// I2C句柄指针
    uint8_t is_init;       // 初始化状态标志
} IST8130_HandleTypeDef1;

// ********** 函数声明 **********
HAL_StatusTypeDef IST8130_Init(IST8130_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef IST8130_ReadID(IST8130_HandleTypeDef *handle, uint8_t *id);
HAL_StatusTypeDef IST8130_SetMode(IST8130_HandleTypeDef *handle, uint8_t mode);
HAL_StatusTypeDef IST8130_ReadData(IST8130_HandleTypeDef *handle);
HAL_StatusTypeDef IST8130_CheckDRDY(IST8130_HandleTypeDef *handle, uint8_t *is_ready);
void IST8130_SetDataReadyFlag(IST8130_HandleTypeDef *handle);
void IST8130_ClearDataReadyFlag(IST8130_HandleTypeDef *handle);
uint8_t IST8130_IsDataReady(IST8130_HandleTypeDef *handle);

#endif // IST8130_H