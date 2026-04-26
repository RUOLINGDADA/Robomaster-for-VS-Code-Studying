#ifndef __BMI088_H
#define __BMI088_H


#include "stm32f407xx.h"
#include "spi.h"

#define BMI088_ACC_CHIPID          0x1EU    /* 官方5.3.1 */
#define BMI088_GYRO_CHIPID         0x0FU    /* 官方5.5.1 */

/*========================= 加速度计寄存器 官方5.2 =========================*/
#define BMI088_ACC_CHIP_ID_REG      0x00U
#define BMI088_ACC_ERR_REG          0x02U
#define BMI088_ACC_STATUS_REG       0x03U
#define BMI088_ACC_DATA_START_REG   0x12U
#define BMI088_ACC_TEMP_MSB_REG     0x22U
#define BMI088_ACC_TEMP_LSB_REG     0x23U
#define BMI088_ACC_CONF_REG         0x40U
#define BMI088_ACC_RANGE_REG        0x41U
#define BMI088_ACC_SELF_TEST_REG    0x6DU
#define BMI088_ACC_PWR_CONF_REG     0x7CU
#define BMI088_ACC_PWR_CTRL_REG      0x7DU
#define BMI088_ACC_SOFTRESET_REG    0x7EU

/*========================= 陀螺仪寄存器 官方5.4 =========================*/
#define BMI088_GYRO_CHIP_ID_REG     0x00U
#define BMI088_GYRO_DATA_START_REG  0x02U
#define BMI088_GYRO_RANGE_REG       0x0FU
#define BMI088_GYRO_BANDWIDTH_REG   0x10U
#define BMI088_GYRO_LPM1_REG        0x11U
#define BMI088_GYRO_SOFTRESET_REG   0x14U
#define BMI088_GYRO_SELF_TEST_REG 0x3CU

// 驱动状态
typedef enum
{
    BMI088_OK       = 0x00U,
    BMI088_ERROR    = 0x01U,
    BMI088_TIMEOUT  = 0x03U
} BMI088_StatusTypeDef;

// 加速度计量程
typedef enum
{
    BMI088_ACC_RANGE_3G   = 0x00U,
    BMI088_ACC_RANGE_6G   = 0x01U,
    BMI088_ACC_RANGE_12G  = 0x02U,
    BMI088_ACC_RANGE_24G  = 0x03U
} BMI088_AccRangeTypeDef;

// 陀螺仪量程
typedef enum
{
    BMI088_GYRO_RANGE_2000DPS = 0x00U,
    BMI088_GYRO_RANGE_1000DPS = 0x01U,
    BMI088_GYRO_RANGE_500DPS  = 0x02U,
    BMI088_GYRO_RANGE_250DPS  = 0x03U,
    BMI088_GYRO_RANGE_125DPS  = 0x04U
} BMI088_GyroRangeTypeDef;

// 加速度计ODR
typedef enum
{
    BMI088_ACC_ODR_12_5HZ = 0x05U,
    BMI088_ACC_ODR_25HZ   = 0x06U,
    BMI088_ACC_ODR_50HZ   = 0x07U,
    BMI088_ACC_ODR_100HZ  = 0x08U,
    BMI088_ACC_ODR_200HZ  = 0x09U,
    BMI088_ACC_ODR_400HZ  = 0x0AU,
    BMI088_ACC_ODR_800HZ  = 0x0BU,
    BMI088_ACC_ODR_1600HZ = 0x0CU
} BMI088_AccOdrTypeDef;

// 陀螺仪带宽/ODR
typedef enum
{
    BMI088_GYRO_BW_532HZ_2000HZ = 0x00U,
    BMI088_GYRO_BW_230HZ_2000HZ = 0x01U,
    BMI088_GYRO_BW_116HZ_1000HZ = 0x02U,
    BMI088_GYRO_BW_47HZ_400HZ   = 0x03U,
    BMI088_GYRO_BW_23HZ_200HZ   = 0x04U,
    BMI088_GYRO_BW_12HZ_100HZ   = 0x05U
} BMI088_GyroBwTypeDef;

// 原始数据
typedef struct
{
    int16_t Acc_X, Acc_Y, Acc_Z;
    int16_t Gyro_X, Gyro_Y, Gyro_Z;
    int16_t Temp_Raw;
} BMI088_RawDataTypeDef;

// 物理单位数据(转化数据)
typedef struct
{
    float Acc_X;    /* mg */
    float Acc_Y;
    float Acc_Z;
    float Gyro_X;   /* dps */
    float Gyro_Y;
    float Gyro_Z;
    float Temp;     /* ℃ */
} BMI088_PhysDataTypeDef;

// BMI088硬件句柄
typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef      *ACC_CS_GPIO;
    uint16_t           ACC_CS_PIN;
    GPIO_TypeDef      *GYRO_CS_GPIO;
    uint16_t GYRO_CS_PIN;
    // 缓存当前配置的量程，避免每次转换都读寄存器
    BMI088_AccRangeTypeDef  AccRange;
    BMI088_GyroRangeTypeDef GyroRange;
} BMI088_HandleTypeDef;

// 对外函数声明
BMI088_StatusTypeDef BMI088_Init(BMI088_HandleTypeDef *hbmi,
                                 BMI088_AccRangeTypeDef acc_range,
                                 BMI088_AccOdrTypeDef acc_odr,
                                 BMI088_GyroRangeTypeDef gyro_range,
                                 BMI088_GyroBwTypeDef gyro_bw);
BMI088_StatusTypeDef BMI088_ReadRaw(BMI088_HandleTypeDef *hbmi,
                                    BMI088_RawDataTypeDef *raw);
BMI088_StatusTypeDef BMI088_ConvertRawToPhys(BMI088_HandleTypeDef *hbmi,
        BMI088_RawDataTypeDef *raw,
        BMI088_PhysDataTypeDef *phys);

#endif
