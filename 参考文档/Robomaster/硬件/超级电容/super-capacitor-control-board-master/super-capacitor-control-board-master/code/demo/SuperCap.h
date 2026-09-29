#ifndef __SUPERCUP_H_
#define __SUPERCUP_H_

#include "main.h"
#include "Can.h"
/*
    CAN通讯发送接口
    SupercupCanTxMsg：CAN发送配置结构体
    SupercupCanTransmit：CAN发送函数
 */

#define SuperCapCanTxMsg CanTxMsg
#define SuperCapCanTransmit(TxMassage, DataBuffer) CAN_Transmit(CAN1, TxMassage)
#define SuperCapTimestamp portTickType
#define SuperCap_Get_Timestamp() xTaskGetTickCount()
#define SUPERCAP_CONNECT_TIMEOUT TIME_STAMP_1000MS

typedef enum
{
    NormalMode,
    OnlyChangeMode,
    OnlyCompensateMode
} Power_Limit_mode_t;

/*
    超级电容数据结构体
*/
typedef struct
{
    struct
    {
        uint16_t Vcap;    // 电容电压
        int16_t Pchassis; // 底盘功率
        int16_t Pcharge;  // 电容充电功率
        uint8_t Vchassis; // 底盘电压
    } feedbuck_value;     // 反馈值
    struct
    {
        uint8_t power_limit;              // 底盘功率上限
        uint8_t power_compensation_limit; // 补偿功率上限
        uint8_t power_charge_limit;       // 充电功率上限
    } config_value;                       // 设置值
    union
    {
        uint8_t value;
        struct
        {
            FunctionalState CapEnable : 1;              // 电容使能
            FunctionalState CapUndervoltage : 1;        // 电容欠压
            FunctionalState PowerCompensationLimit : 1; // 补偿达到上限
            FunctionalState RESERVER : 4;               // 保留位
            FunctionalState PowerLoopError : 1;         // 电源环路异常
        } flag;
    } status; // 状态值
    union
    {
        uint8_t value;
        struct
        {
            FunctionalState CupEnable : 1;
            Power_Limit_mode_t power_limit_mode : 2;
            FunctionalState RESERVER : 5;
        } flag;
    } tx_status;
    SuperCapTimestamp rx_timestamp; // 超级电容接收时间戳
} SuperCap_t;
/* 变量 */
extern SuperCap_t supercap; // 超级电容数据结构体
/* 函数 */
/// @brief 设置底盘期望功率
#define Set_SuperCap_PowerLimit(Power_Limit) (supercap.config_value.power_limit = (Power_Limit))
/// @brief 设置电容补偿功率
#define Set_SuperCap_CompensationLimit(Compensation_Limit) (supercap.config_value.power_compensation_limit = (Compensation_Limit))
/// @brief 设置电容充电功率
#define Set_SuperCap_ChargeLimit(Charge_Limit) (supercap.config_value.power_charge_limit = (Charge_Limit))
/// @brief 读取电容电压
#define Get_SuperCap_Vcap() (supercap.feedbuck_value.Vcap * 0.01f)
/// @brief 读取底盘功率
#define Get_SuperCap_Pchassis() (supercap.feedbuck_value.Pchassis * 0.01f)
/// @brief 读取底盘电压
#define Get_SuperCap_Vchassis() (supercap.feedbuck_value.Vchassis * 0.01f)
/// @brief 读取充电功率
#define Get_SuperCap_Pcharge() (supercap.feedbuck_value.Pcharge * 0.01f)
/// @brief 读取电容状态
/// @param flag_name 状态名称，例如获取电容使能情况：Get_SuperCap_Status(CapEnable)
#define Get_SuperCap_Status(flag_name) (supercap.status.flag.##flag_name)

void Receive_SuperCap_Feedback(uint8_t supercap_rxbuffer[8]);
void SuperCap_Sand_Config(SuperCapCanTxMsg *tx_message, uint8_t supercap_txbuffer[8], FunctionalState supercap_state, Power_Limit_mode_t Power_limit_mode);
void SuperCup_Sand_Data(SuperCapCanTxMsg *tx_message, uint8_t supercup_txbuffer[8], uint16_t buffer_power);
ErrorStatus Get_SuperCap_Loss_Of_Connection();
#endif