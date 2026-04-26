#include "bmi088.h"

static void BMI088_ACC_CS_Low(BMI088_HandleTypeDef *hbmi);
static void BMI088_ACC_CS_High(BMI088_HandleTypeDef *hbmi);
static void BMI088_GYRO_CS_Low(BMI088_HandleTypeDef *hbmi);
static void BMI088_GYRO_CS_High(BMI088_HandleTypeDef *hbmi);

static BMI088_StatusTypeDef BMI088_ACC_WriteReg(BMI088_HandleTypeDef *hbmi, uint8_t reg, uint8_t val);
static BMI088_StatusTypeDef BMI088_ACC_ReadReg(BMI088_HandleTypeDef *hbmi, uint8_t reg, uint8_t *val);
static BMI088_StatusTypeDef BMI088_GYRO_WriteReg(BMI088_HandleTypeDef *hbmi, uint8_t reg, uint8_t val);
static BMI088_StatusTypeDef BMI088_GYRO_ReadReg(BMI088_HandleTypeDef *hbmi,
        uint8_t reg, uint8_t *val);

static void BMI088_ACC_CS_Low(BMI088_HandleTypeDef *hbmi)
{
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, RESET);
}

static void BMI088_ACC_CS_High(BMI088_HandleTypeDef *hbmi)
{
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, SET);
}

static void BMI088_GYRO_CS_Low(BMI088_HandleTypeDef *hbmi)
{
    HAL_GPIO_WritePin(hbmi->GYRO_CS_GPIO, hbmi->GYRO_CS_PIN, RESET);
}

static void BMI088_GYRO_CS_High(BMI088_HandleTypeDef *hbmi)
{
    HAL_GPIO_WritePin(hbmi->GYRO_CS_GPIO, hbmi->GYRO_CS_PIN, SET);
}

static BMI088_StatusTypeDef BMI088_ACC_WriteReg(BMI088_HandleTypeDef *hbmi,
        uint8_t reg, uint8_t val)
{
    // 0x7F: 0111 1111
    uint8_t tx[2] = {reg & 0x7FU, val};

    BMI088_ACC_CS_Low(hbmi);
    if (HAL_SPI_Transmit(hbmi->hspi, tx, 2U, 199U) != HAL_OK)
    {
        BMI088_ACC_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_ACC_CS_High(hbmi);

    /*
     * BMI088写寄存器后需要等待内部同步。
     * normal mode 至少2us，suspend mode 至少1000us。
     * 这里直接1ms，先保证稳定。
     */
    HAL_Delay(1);

    return BMI088_OK;
}

static BMI088_StatusTypeDef BMI088_ACC_ReadReg(BMI088_HandleTypeDef *hbmi,
        uint8_t reg, uint8_t *val)
{
    // 0x80: 1000 0000
    uint8_t tx[3] = {reg | 0x80U, 0x00U, 0x00U};
    uint8_t rx[3] = {0};

    BMI088_ACC_CS_Low(hbmi);
    if (HAL_SPI_TransmitReceive(hbmi->hspi, tx, rx, 3U, 100U) != HAL_OK)
    {
        BMI088_ACC_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_ACC_CS_High(hbmi);

    /**
    6.1.2 SPI interface of accelerometer part
    In case of read operations of the accelerometer part, the requested data is not sent immediately, but
    instead first a dummy byte is sent, and after this dummy byte the actual reqested register content is
    transmitted.
    */
    *val = rx[2];

    return BMI088_OK;
}

static BMI088_StatusTypeDef BMI088_GYRO_WriteReg(BMI088_HandleTypeDef *hbmi,
        uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = {reg & 0x7FU, val};

    BMI088_GYRO_CS_Low(hbmi);
    if (HAL_SPI_Transmit(hbmi->hspi, tx, 2U, 100U) != HAL_OK)
    {
        BMI088_GYRO_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_GYRO_CS_High(hbmi);

    /*
     * BMI088写寄存器后需要等待内部同步。
     * gyro normal mode 下2us即可，这里1ms，先保证稳定。
     */
    HAL_Delay(1);

    return BMI088_OK;
}

static BMI088_StatusTypeDef BMI088_GYRO_ReadReg(BMI088_HandleTypeDef *hbmi,
        uint8_t reg, uint8_t *val)
{
    uint8_t tx[2] = {reg | 0x80U, 0x00U};
    uint8_t rx[2] = {0};

    BMI088_GYRO_CS_Low(hbmi);
    if (HAL_SPI_TransmitReceive(hbmi->hspi, tx, rx, 2U, 100U) != HAL_OK)
    {
        BMI088_GYRO_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_GYRO_CS_High(hbmi);

    *val = rx[1];

    return BMI088_OK;
}

BMI088_StatusTypeDef BMI088_Init(BMI088_HandleTypeDef *hbmi,
                                 BMI088_AccRangeTypeDef acc_range,
                                 BMI088_AccOdrTypeDef acc_odr,
                                 BMI088_GyroRangeTypeDef gyro_range,
                                 BMI088_GyroBwTypeDef gyro_bw)
{
    if (hbmi == NULL || hbmi->hspi == NULL)
    {
        return BMI088_ERROR;
    }

    uint8_t id = 0;

    BMI088_ACC_CS_High(hbmi);
    BMI088_GYRO_CS_High(hbmi);
    HAL_Delay(10);

    /*
     * 加速度计上电默认是I2C模式。
     * 需要ACC_CS产生一次上升沿，然后做一次dummy SPI读，切换到SPI模式。
     */
    // 切换加速度计SPI模式
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    // dummy读
    /**
    To change the accelerometer to SPI mode in the initialization phase, the
    user could perform a dummy SPI read operation, e.g. of register ACC_CHIP_ID
    (the obtained value will be invalid).
    */
    BMI088_ACC_ReadReg(hbmi, BMI088_ACC_CHIP_ID_REG, &id);
    HAL_Delay(1);

    // 软复位两个传感器
    BMI088_ACC_WriteReg(hbmi, BMI088_ACC_SOFTRESET_REG, 0xB6);
    HAL_Delay(10);

    /*
     * 加速度计soft reset后，接口可能重新回到默认状态。
     * 所以这里再次切换加速度计SPI模式。
     */
    // 切换加速度计SPI模式
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(hbmi->ACC_CS_GPIO, hbmi->ACC_CS_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    // dummy读
    /**
    To change the accelerometer to SPI mode in the initialization phase, the
    user could perform a dummy SPI read operation, e.g. of register ACC_CHIP_ID
    (the obtained value will be invalid).
    */
    BMI088_ACC_ReadReg(hbmi, BMI088_ACC_CHIP_ID_REG, &id);
    HAL_Delay(1);

    BMI088_GYRO_WriteReg(hbmi, BMI088_GYRO_SOFTRESET_REG, 0xB6);
    HAL_Delay(30);

    // 加速度计ID校验
    if (BMI088_ACC_ReadReg(hbmi, BMI088_ACC_CHIP_ID_REG, &id) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    if (id != BMI088_ACC_CHIPID)
    {
        return BMI088_ERROR;
    }

    // 陀螺仪ID校验
    if (BMI088_GYRO_ReadReg(hbmi, BMI088_GYRO_CHIP_ID_REG, &id) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    if (id != BMI088_GYRO_CHIPID)
    {
        return BMI088_ERROR;
    }



    // 加速计配置
    // 配置加速计RANGE
    if (BMI088_ACC_WriteReg(hbmi, BMI088_ACC_RANGE_REG, acc_range) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2);

    // 配置加速计BWP(NORMAL) ODR
    if (BMI088_ACC_WriteReg(hbmi, BMI088_ACC_CONF_REG,
                            (0x0AU << 4U) | acc_odr) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2);

    // 加速计上电
    if (BMI088_ACC_WriteReg(hbmi, BMI088_ACC_PWR_CTRL_REG, 0x04U) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2); /* 等待450us+ */

    if (BMI088_ACC_WriteReg(hbmi, BMI088_ACC_PWR_CONF_REG, 0x00U) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2);

    // 配置陀螺仪
    // 配置陀螺仪RANGE
    if (BMI088_GYRO_WriteReg(hbmi, BMI088_GYRO_RANGE_REG, gyro_range) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2);

    // 配置陀螺仪BW
    if (BMI088_GYRO_WriteReg(hbmi, BMI088_GYRO_BANDWIDTH_REG, gyro_bw) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(2);

    // 配置陀螺仪电源模式(NORMAL)
    if (BMI088_GYRO_WriteReg(hbmi, BMI088_GYRO_LPM1_REG, 0x00U) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    HAL_Delay(30);

    hbmi->AccRange = acc_range;
    hbmi->GyroRange = gyro_range;

    return BMI088_OK;
}

static inline int16_t BMI088_MakeInt16(uint8_t lsb, uint8_t msb)
{
    return (int16_t)(((uint16_t)msb << 8U) | lsb);
}

BMI088_StatusTypeDef BMI088_ReadRaw(BMI088_HandleTypeDef *hbmi,
                                    BMI088_RawDataTypeDef *raw)
{
    if (hbmi == NULL || hbmi->hspi == NULL || raw == NULL)
    {
        return BMI088_ERROR;
    }

    // 加速度计Burst读取
    uint8_t tx_acc[8] = {BMI088_ACC_DATA_START_REG | 0x80U, 0, 0, 0, 0, 0, 0, 0};
    uint8_t rx_acc[8] = {0}; // 1字节地址响应 + 1字节dummy + 6字节数据

    BMI088_ACC_CS_Low(hbmi);
    if (HAL_SPI_TransmitReceive(hbmi->hspi, tx_acc, rx_acc, 8U, 100U) != HAL_OK)
    {
        BMI088_ACC_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_ACC_CS_High(hbmi);

    // 解析数据：rx_acc[0]无效，rx_acc[1]是dummy，从rx_acc[2]开始是0x12~0x17
    raw->Acc_X = BMI088_MakeInt16(rx_acc[2], rx_acc[3]);
    raw->Acc_Y = BMI088_MakeInt16(rx_acc[4], rx_acc[5]);
    raw->Acc_Z = BMI088_MakeInt16(rx_acc[6], rx_acc[7]);

    // 陀螺仪Burst读取
    uint8_t tx_gyro[7] = {BMI088_GYRO_DATA_START_REG | 0x80U, 0, 0, 0, 0, 0, 0};
    uint8_t rx_gyro[7] = {0};

    BMI088_GYRO_CS_Low(hbmi);
    if (HAL_SPI_TransmitReceive(hbmi->hspi, tx_gyro, rx_gyro, 7U, 100U) != HAL_OK)
    {
        BMI088_GYRO_CS_High(hbmi);
        return BMI088_ERROR;
    }
    BMI088_GYRO_CS_High(hbmi);

    raw->Gyro_X = BMI088_MakeInt16(rx_gyro[1], rx_gyro[2]);
    raw->Gyro_Y = BMI088_MakeInt16(rx_gyro[3], rx_gyro[4]);
    raw->Gyro_Z = BMI088_MakeInt16(rx_gyro[5], rx_gyro[6]);

    uint8_t t_msb = 0, t_lsb = 0;
    if (BMI088_ACC_ReadReg(hbmi, BMI088_ACC_TEMP_MSB_REG, &t_msb) != BMI088_OK)
    {
        return BMI088_ERROR;
    }
    if (BMI088_ACC_ReadReg(hbmi, BMI088_ACC_TEMP_LSB_REG, &t_lsb) != BMI088_OK)
    {
        return BMI088_ERROR;
    }

    raw->Temp_Raw = (int16_t)(((uint16_t)t_msb << 3U) | (t_lsb >> 5U));

    return BMI088_OK;
}

BMI088_StatusTypeDef BMI088_ConvertRawToPhys(BMI088_HandleTypeDef *hbmi,
        BMI088_RawDataTypeDef *raw,
        BMI088_PhysDataTypeDef *phys)
{
    if (hbmi == NULL || raw == NULL || phys == NULL)
    {
        return BMI088_ERROR;
    }

    // 加速计
    // 官方固定满量程：16位补码 ±32767
    // (1 << n) 就是计算 2ⁿ
    const float acc_full_scale = 32768.0f;
    float acc_range_g; // acc_range_g = 1.5f * (1 << (range + 1));

    switch (hbmi->AccRange)
    {
    case BMI088_ACC_RANGE_3G:  acc_range_g = 3.0f;   break;
    case BMI088_ACC_RANGE_6G:  acc_range_g = 6.0f;   break;
    case BMI088_ACC_RANGE_12G: acc_range_g = 12.0f;  break;
    case BMI088_ACC_RANGE_24G: acc_range_g = 24.0f;  break;
    default: acc_range_g = 6.0f; break;
    }

    phys->Acc_X = (float)raw->Acc_X / acc_full_scale * 1000.0f * acc_range_g;
    phys->Acc_Y = (float)raw->Acc_Y / acc_full_scale * 1000.0f * acc_range_g;
    phys->Acc_Z = (float)raw->Acc_Z / acc_full_scale * 1000.0f * acc_range_g;

    // 陀螺仪
    // 官方固定满量程：16位补码 ±32767
    const float gyro_full_scale = 32767.0f;
    float gyro_range_dps;

    switch (hbmi->GyroRange)
    {
    case BMI088_GYRO_RANGE_2000DPS: gyro_range_dps = 2000.0f; break;
    case BMI088_GYRO_RANGE_1000DPS: gyro_range_dps = 1000.0f; break;
    case BMI088_GYRO_RANGE_500DPS:  gyro_range_dps = 500.0f;  break;
    case BMI088_GYRO_RANGE_250DPS:  gyro_range_dps = 250.0f;  break;
    case BMI088_GYRO_RANGE_125DPS:  gyro_range_dps = 125.0f;  break;
    default: gyro_range_dps = 2000.0f; break;
    }

    // 官方公式实现：原始值 / 32767 * 量程
    phys->Gyro_X = (float)raw->Gyro_X / gyro_full_scale * gyro_range_dps;
    phys->Gyro_Y = (float)raw->Gyro_Y / gyro_full_scale * gyro_range_dps;
    phys->Gyro_Z = (float)raw->Gyro_Z / gyro_full_scale * gyro_range_dps;

    // 温度
    int16_t temp = raw->Temp_Raw;
    if (temp > 1023)
    {
        temp -= 2048;
    }
    phys->Temp = 23.0f + (float)temp * 0.125f;

    return BMI088_OK;
}