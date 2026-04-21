#ifndef BSP_IST8130_H
#define BSP_IST8130_H

#include "ist8130.h"


#define IST8130_DEV_ADDR          0x0E<<1  // 7位地址0x0E，左移1位含读写位
#define IST8130_WHO_AM_I          0x00     // 设备ID寄存器地址 (ID固定:0x10)
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

typedef struct
{
    I2C_HandleTypeDef *hi2c;
    int16_t mag_x;         // X轴磁场数据（单位：uT）
    int16_t mag_y;         // Y轴磁场数据（单位：uT）
    int16_t mag_z;         // Z轴磁场数据（单位：uT）
    uint8_t status;        // 状态标志（bit0:数据就绪）
    uint8_t is_init;       // 初始化状态标志
} IST8130_HandleTypeDef;

HAL_StatusTypeDef BSP_IST8130_Init(IST8130_HandleTypeDef *handle, I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef BSP_IST8130_DeInit(IST8130_HandleTypeDef *handle);


#endif