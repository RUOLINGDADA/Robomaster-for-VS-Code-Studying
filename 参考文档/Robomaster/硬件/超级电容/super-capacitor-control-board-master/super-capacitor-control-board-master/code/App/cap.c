#include "cap.h"

__attribute__((section("ccmram"))) CAP_data_t Cap_data = 
{
	.energy_data = {
		.cap_coulomb_value = 0,
		.cap_energy_max = 0,
		.cap_energy_value = 0
	},
	.parameter_data =
	{
		.cap_resistance = 0,
		.cap_value = 0
	}
};


/// @brief 电容电荷量计算
/// @param[out] coulomb_value 电荷量(单位:C)
/// @param[in] I_value 电容电流(单位:A)
/// @param[in] samp_time 采样时间(单位:S)
/// @attention 电容电流方向为对电容充电的电流方向为正方向
__attribute__((section("ccmram"))) void Coulomb_Calculation(float *coulomb_value, float I_value, float samp_time)
{
	float temp = *coulomb_value;
	temp += I_value * samp_time;
	if (temp < 0.0f)
	{
		temp = 0;
	}
	*coulomb_value = temp;
}

/// @brief 电容量计算
/// @param[in] coulomb_value 电荷量(单位:C)
/// @param[in] Vcap 电容电压(单位:V)
/// @return 电容量(单位:F)
__attribute__((section("ccmram"))) float Cap_Value_Calculation(float coulomb_value, float Vcap)
{
	static float cap_value;
	if (coulomb_value < 5)
	{
		return cap_value;
	}
	cap_value = coulomb_value / Vcap;
	return cap_value;
}
/// @brief 电容电量计算
/// @param[out] energy_max 电量最大值
/// @param[in] Vcap 电容电压
/// @param[in] coulomb_value 电荷量
/// @return 当前电量
__attribute__((section("ccmram"))) float Cap_Energy_Calculation(float *energy_max, float Vcap, float coulomb_value)
{
	float energy_now = Vcap * coulomb_value * 0.652f;

	// 更新电容电量
	if (energy_now > *energy_max)
	{
		*energy_max = energy_now;
	}
	return energy_now;
}