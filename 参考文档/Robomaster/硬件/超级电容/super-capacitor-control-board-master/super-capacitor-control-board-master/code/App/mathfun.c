#include "mathfun.h"

/// @brief 限幅函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 限幅结果
__attribute__((section("ccmram"))) float Constrain_float(float amt, float high, float low)
{
	if (amt < low)
			return low;
	else if (amt > high)
			return high;
	else
			return amt;
}
/// @brief 限幅函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 限幅结果
__attribute__((section("ccmram"))) int16_t Constrain_int16(int16_t amt, int16_t high, int16_t low)
{
	if (amt < low)
			return low;
	else if (amt > high)
			return high;
	else
			return amt;
}
/// @brief 限幅函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 限幅结果
__attribute__((section("ccmram"))) int32_t Constrain_int32(int32_t amt, int32_t high, int32_t low)
{
	if (amt < low)
			return low;
	else if (amt > high)
			return high;
	else
			return amt;
}

/// @brief 死区函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 死区结果
__attribute__((section("ccmram"))) float DeadZone_float(float amt,float high,float low)
{
	if (amt > low && amt < high)
		return 0;
	else
		return amt;
}

/// @brief 死区函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 死区结果
__attribute__((section("ccmram"))) int16_t DeadZone_int16(int16_t amt,int16_t high,int16_t low)
{
	if (amt > low && amt < high)
		return 0;
	else
		return amt;
}

/// @brief 死区函数
/// @param amt 输入值
/// @param high 最大值
/// @param low 最小值
/// @return 死区结果
__attribute__((section("ccmram"))) int32_t DeadZone_int32(int32_t amt,int32_t high,int32_t low)
{
	if (amt > low && amt < high)
		return 0;
	else
		return amt;
}

/// @brief 反死区函数
/// @param amt 输入值
/// @param positive_offset 正偏置值
/// @param negative_offset 负偏置值
/// @return 反死区结果
/// @note 对应Simulink的Coulomb and Viscous Friction模块
__attribute__((section("ccmram"))) float Anti_DeadZone_float(float amt, float positive_offset, float negative_offset)
{
	if(amt > 0)
	{
		return amt + positive_offset;
	}
	else if(amt < 0)
	{
		return amt + negative_offset;
	}
	else
	{
		return 0;
	}
}

/// @brief 反死区函数
/// @param amt 输入值
/// @param positive_offset 正偏置值
/// @param negative_offset 负偏置值
/// @return 反死区结果
/// @note 对应Simulink的Coulomb and Viscous Friction模块
__attribute__((section("ccmram"))) int32_t Anti_DeadZone_int32(int32_t amt, int32_t positive_offset, int32_t negative_offset)
{
	if(amt > 0)
	{
		return amt + positive_offset;
	}
	else if(amt < 0)
	{
		return amt + negative_offset;
	}
	else
	{
		return 0;
	}
}

/// @brief 反死区函数
/// @param amt 输入值
/// @param positive_offset 正偏置值
/// @param negative_offset 负偏置值
/// @return 反死区结果
/// @note 对应Simulink的Coulomb and Viscous Friction模块
__attribute__((section("ccmram"))) int16_t Anti_DeadZone_int16(int16_t amt, int16_t positive_offset, int16_t negative_offset)
{
	if(amt > 0)
	{
		return amt + positive_offset;
	}
	else if(amt < 0)
	{
		return amt + negative_offset;
	}
	else
	{
		return 0;
	}
}

/// @brief 斜坡函数
/// @param target_value 期望值
/// @param current_value 当前值
/// @param step_length 步长
/// @return 本次迭代结果
__attribute__((section("ccmram"))) float Ramp_float(float target_value,float current_value,float step_length)
{
	float s = sign(target_value - current_value);
	float temp = current_value + s * step_length;
	if(s*(temp - target_value)>=0)
	{
		return target_value;
	}
	else
	{
		return temp;
	}
}

/// @brief 斜坡函数
/// @param target_value 期望值
/// @param current_value 当前值
/// @param step_length 步长
/// @return 本次迭代结果
__attribute__((section("ccmram"))) int32_t Ramp_int32(int32_t target_value,int32_t current_value,int32_t step_length)
{
	int32_t temp = current_value + sign(target_value - current_value) * step_length;
	if(temp >= target_value)
	{
		return target_value;
	}
	else
	{
		return temp;
	}
}

/// @brief 斜坡函数
/// @param target_value 期望值
/// @param current_value 当前值
/// @param step_length 步长
/// @return 本次迭代结果
__attribute__((section("ccmram"))) int16_t Ramp_int16(int16_t target_value,int16_t current_value,int16_t step_length)
{
	int16_t temp = current_value + sign(target_value - current_value) * step_length;
	if(temp >= target_value)
	{
		return target_value;
	}
	else
	{
		return temp;
	}
}