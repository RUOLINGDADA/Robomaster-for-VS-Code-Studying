#include "sample.h"
#include "adc.h"
#include "dma.h"
//使用DMA的变量时候不能将其放在CCM RAM中!!!
uint16_t ADC1Result[4];
uint16_t ADC2Result[2];
__attribute__((section("ccmram"))) ADC_Data_t Vcap, Vin, Vout, Iin, Iout, Icap;

/// @brief 配置ADC参数
/// @param ADC_value ADC数据结构体
/// @param ADC_buffer ADC输出数组
/// @param proportion 线性矫正K值
/// @param offset 线性矫正B值
/// @param lowpass_a 低通滤波值
void ADC_Data_Config(ADC_Data_t *ADC_value, uint16_t *ADC_buffer, float proportion, float offset, float lowpass_a, ADC_type_t type)
{
	ADC_value->ave_value = 0;
	ADC_value->now_value = 0;
	ADC_value->lowpass_a = lowpass_a;
	ADC_value->conversion.offset = offset;
	ADC_value->conversion.proportion = proportion;
	ADC_value->ADC_value = ADC_buffer;
	ADC_value->type = type;
}
/// @brief 初始化ADC数据结构体
void Init_ADC_Data(void)
{
	ADC_Data_Config(&Vin, &ADC1Result[ADC_VIN], CAL_VIN_K, CAL_VIN_B, VIN_LOWPASS_A, type_voltage);
	ADC_Data_Config(&Vout, &ADC1Result[ADC_VOUT], CAL_VOUT_K, CAL_VOUT_B, VOUT_LOWPASS_A, type_voltage);
	ADC_Data_Config(&Vcap, &ADC1Result[ADC_VCAP], CAL_VCAP_K, CAL_VCAP_B, VCAP_LOWPASS_A, type_voltage);
	ADC_Data_Config(&Icap, &ADC1Result[ADC_ICAP], CAL_ICAP_K, CAL_ICAP_B, ICAP_LOWPASS_A, type_current);
	ADC_Data_Config(&Iin, &ADC2Result[ADC_IIN], CAL_IIN_K, CAL_IIN_B, IIN_LOWPASS_A, type_current);
	ADC_Data_Config(&Iout, &ADC2Result[ADC_IOUT], CAL_IOUT_K, CAL_IOUT_B, IOUT_LOWPASS_A, type_current);
}
void Init_ADC_Config(void)
{
    //ADC采样校准
    HAL_ADCEx_Calibration_Start(&hadc1,ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2,ADC_SINGLE_ENDED);
    //ADC和DMA初始化
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC1Result, 4); // 启动ADC1采样 DMA数据传送采样输入输出电压电流
    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADC2Result, 2); // 启动ADC2采样 DMA数据传送采样输入输出电压电流
}

/// @brief 获取ADC数值
/// @param ADC ADC数据存储结构体
__attribute__((section("ccmram"))) void Get_ADC(ADC_Data_t *ADC)
{
	ADC->now_value = ADC->conversion.proportion * *(ADC->ADC_value) + ADC->conversion.offset;
	ADC->ave_value = LowPass(ADC->now_value, ADC->ave_value, ADC->lowpass_a);
}

/// @brief 解算ADC数值
/// @param
__attribute__((section("ccmram"))) void Resolve_ADC(void)
{
	Get_ADC(&Vout);
	Get_ADC(&Vcap);
	Get_ADC(&Iin);
	Get_ADC(&Iout);
	Get_ADC(&Icap);
#if DELETE_VIN_SAMPLE
	Vin.ADC_value = Vout.ADC_value;
	Vin.ave_value = Vout.ave_value;
	Vin.now_value = Vout.now_value;
#else
	Get_ADC(&Vin);
#endif
}