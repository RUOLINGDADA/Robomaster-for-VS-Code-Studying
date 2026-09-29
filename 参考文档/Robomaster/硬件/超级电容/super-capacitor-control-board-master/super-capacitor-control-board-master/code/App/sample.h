#ifndef _SAMPLE_H_
#define _SAMPLE_H_
#include "main.h"
// ADC矫正参数
#define CAL_VOUT_K 0.016487241f  // 输出电压矫正比例系数
#define CAL_VOUT_B 0.239850047f  // 输出电压矫正偏置系数
#define CAL_VIN_K 0.016487241f   // 输入电压矫正比例系数
#define CAL_VIN_B 0.239850047f   // 输入电压矫正偏置系数
#define CAL_VCAP_K 0.016487241f  // 电容电压矫正比例系数
#define CAL_VCAP_B 0.239850047f  // 电容电压矫正偏置系数
#define CAL_IOUT_K -0.007823744f // 输出电流矫正比例系数
#define CAL_IOUT_B 15.96915972f  // 输出电流矫正偏置系数
#define CAL_IIN_K 0.004099898f   // 输入电流矫正比例系数
#define CAL_IIN_B -8.288084255f  // 输入电流矫正偏置系数
#define CAL_ICAP_K -0.008195741f // 电容电流矫正比例系数
#define CAL_ICAP_B 16.57911111f  // 电容电流矫正偏置系数

#define VIN_LOWPASS_A 0.1f   // 输入电压低通滤波系数
#define VOUT_LOWPASS_A 0.1f  // 输出电压低通滤波系数
#define VCAP_LOWPASS_A 0.1f  // 电容电压低通滤波系数
#define IIN_LOWPASS_A 0.08f  // 输入电流低通滤波系数
#define IOUT_LOWPASS_A 0.08f // 输出电流低通滤波系数
#define ICAP_LOWPASS_A 0.08f // 电容电流低通滤波系数
#define PCAP_LOWPASS_A 0.1f  // 电容功率低通滤波系数
#define PIN_LOWPASS_A 0.08f  // 输入功率低通滤波系数
#define POUT_LOWPASS_A 0.08f // 输出功率低通滤波系数
typedef enum
{
    ADC_VOUT = 0,
    ADC_VIN,
    ADC_VCAP,
    ADC_ICAP
} ADC1_Value;
typedef enum
{
    ADC_IIN = 0,
    ADC_IOUT
} ADC2_Value;
typedef enum
{
    type_voltage,
    type_current
} ADC_type_t;
typedef struct
{
    uint16_t *ADC_value; // ADC结果指针
    struct
    {
        float proportion; // 比例系数
        float offset;     // 偏置值
    } conversion;         // 线性修正参数 proportion*x+offset
    float now_value;      // 当前值
    float ave_value;      // 平均值
    float lowpass_a;      // 低通滤波系数
    ADC_type_t type;
} ADC_Data_t;

extern uint16_t ADC1Result[4];
extern uint16_t ADC2Result[2];
extern ADC_Data_t Vcap, Vin, Vout, Iin, Iout, Icap;

void ADC_Data_Config(ADC_Data_t *ADC_value, uint16_t *ADC_buffer, float proportion, float offset, float lowpass_a, ADC_type_t type);
void Get_ADC(ADC_Data_t *ADC);

void Init_ADC_Data(void);
void Resolve_ADC(void);

#endif
/***************************End of File**********************************/
