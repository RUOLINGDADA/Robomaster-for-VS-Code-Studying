#include "Super_Capacitor.h"

SuperCap_t supercap =
    {
        .feedbuck_value =
            {
                .Vcap = 0,
                .Pchassis = 24.0f,
                .Pcharge = 0,
            },
        .config_value =
            {
                .power_limit = 50,
                .power_compensation_limit = 300,
                .power_charge_limit = 150,
            },
        .status.value = 0x00,
};

/// @brief 解算超级电容接收值
/// @param supercap_rxbuffer 接收暂存数组
void Receive_SuperCap_Feedback(uint8_t supercap_rxbuffer[8])
{
    assert_param(supercap_rxbuffer != NULL);

    supercap.feedbuck_value.Vcap = (uint16_t)supercap_rxbuffer[0] << 8 | (uint16_t)supercap_rxbuffer[1];
    supercap.feedbuck_value.Pcharge = (int16_t)supercap_rxbuffer[2] << 8 | (int16_t)supercap_rxbuffer[3];
    supercap.feedbuck_value.Pchassis = (int16_t)supercap_rxbuffer[4] << 8 | supercap_rxbuffer[5];
    supercap.status.value = supercap_rxbuffer[6];
    supercap.feedbuck_value.Vchassis = supercap_rxbuffer[7];
    supercap.rx_timestamp = SuperCap_Get_Timestamp();
}

/// @brief 发送超级电容配置
/// @param[out] supercap_txbuffer 发送数组
/// @param[in] supercap_state 电容开关 ENABLE:开启电容 DISABLE:关闭电容
/// @param[in] Power_limit_mode 功率补偿状态 NormalMode:常规模式 OnlyChangeMode:只充电模式(电容强制吸收10J+的功率)  OnlyCompensateMode:只放电模式(电容只充电不放电，直到没电为止)
/// @attention supercup_txbaffer需要写入，注意大小必须匹配
void SuperCap_Sand_Config(SuperCapCanTxMsg *tx_message, uint8_t supercap_txbuffer[8], FunctionalState supercap_state, Power_Limit_mode_t power_limit_mode)
{
    assert_param(tx_message != NULL);
    assert_param(supercap_txbuffer != NULL);
    assert_param(supercap_state == ENABLE || supercap_state == DISABLE);

    tx_message->StdId = 0x02f;      // 标准帧ID
    tx_message->IDE = CAN_ID_STD;   // 标准模式
    tx_message->RTR = CAN_RTR_DATA; // 发送的是数据帧
    tx_message->DLC = 8;            // 数据长度为8字节
    supercap.tx_status.flag.power_limit_mode = power_limit_mode;
    supercap.tx_status.flag.CupEnable = supercap_state;
    supercap_txbuffer[0] = supercap.config_value.power_limit;
    supercap_txbuffer[1] = 0;
    supercap_txbuffer[2] = supercap.config_value.power_compensation_limit;
    supercap_txbuffer[3] = 0;
    supercap_txbuffer[4] = supercap.config_value.power_charge_limit;
    supercap_txbuffer[5] = 0;
    supercap_txbuffer[6] = supercap.tx_status.value;

    SuperCapCanTransmit(tx_message, supercap_txbuffer);
}

/// @brief 发送超级电容数据
/// @param supercap_txbuffer 发送数组
/// @param buffer_power 裁判系统发来的能量缓冲，该数据放大了100倍
/// @param expect_buffer_power 期望的能量缓冲剩余，该数据放大了100倍
/// @attention supercap_txbuffer需要写入，注意大小必须匹配
void SuperCup_Sand_Data(SuperCapCanTxMsg *tx_message, uint8_t supercap_txbuffer[8], uint16_t buffer_power)
{
    assert_param(tx_message != NULL);
    assert_param(supercap_txbuffer != NULL);

    tx_message->StdId = 0x02e;      // 标准帧ID
    tx_message->IDE = CAN_ID_STD;   // 标准模式
    tx_message->RTR = CAN_RTR_DATA; // 发送的是数据帧
    tx_message->DLC = 8;            // 数据长度为8字节

    supercap_txbuffer[0] = buffer_power >> 8;
    supercap_txbuffer[1] = buffer_power;
    supercap_txbuffer[2] = 0;
    supercap_txbuffer[3] = 0;
    supercap_txbuffer[4] = 0;
    supercap_txbuffer[5] = 0;
    supercap_txbuffer[6] = 0;
    supercap_txbuffer[7] = 0;

    SuperCapCanTransmit(tx_message, supercap_txbuffer);
}
/// @brief 超级电容失联检测
/// @return ERROR:失联 SUCCESS:未失联
ErrorStatus Get_SuperCap_Loss_Of_Connection()
{
    if(SuperCap_Get_Timestamp() - supercap.rx_timestamp > SUPERCAP_CONNECT_TIMEOUT)
    {
        return ERROR;
    }
    else
    {
        return SUCCESS;
    }
}