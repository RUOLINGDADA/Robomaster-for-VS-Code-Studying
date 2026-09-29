#include "PID.h"

#ifndef Mul
#define TEMP_MUL
#define Mul(a,b) ((a)*(b))
#endif


/// @brief PID初始化
/// @param PID_Config PID配置结构体
/// @param K_P 比例因子
/// @param K_I 积分因子
/// @param K_D 微分因子
/// @param P_Max 比例最大值
/// @param I_Max 积分最大值
/// @param D_Max 微分最大值
/// @param PID_Max PID最大值
/// @param PID_Min PID最小值
void PID_Init(PIDConfig_t *PID_Config,PIDElem_t K_P,PIDElem_t K_I,PIDElem_t K_D,PIDElem_t P_Max,PIDElem_t I_Max,PIDElem_t D_Max,PIDElem_t PID_Max,PIDElem_t PID_Min)
{
	PID_Config->KP = K_P;						//比例因子
	PID_Config->KI = K_I;						//积分因子
	PID_Config->KD = K_D;						//微分因子
	PID_Config->PMax = P_Max;					//比例最大值
    PID_Config->IMax = I_Max;					//积分最大值
	PID_Config->DMax = D_Max;					//微分最大值
	PID_Config->PIDMax = PID_Max;				//PID最大值
	PID_Config->PIDMin = PID_Min;				//PID最大值
	PID_Config->IntegralValue = 0;				//积分
	PID_Config->DiffValue = 0;					//误差
}
/// @brief 配置PID积分值
/// @param PID_Config PID配置结构体
/// @param integral_value 积分值
void PID_Integral_Config(PIDConfig_t *PID_Config,PIDElem_t integral_value)
{
	PID_Config->IntegralValue = integral_value;
}
/// @brief 配置PID参数
/// @param PID_Config PID配置结构体
/// @param K_P 比例因子
/// @param K_I 积分因子
/// @param K_D 微分因子
/// @param P_Max 比例最大值
/// @param I_Max 积分最大值
/// @param D_Max 微分最大值
void PID_Config(PIDConfig_t *PID_Config,PIDElem_t K_P,PIDElem_t K_I,PIDElem_t K_D,PIDElem_t P_Max,PIDElem_t I_Max,PIDElem_t D_Max)
{
	PID_Config->KP = K_P;						//比例因子
	PID_Config->KI = K_I;						//积分因子
	PID_Config->KD = K_D;						//微分因子
	PID_Config->PMax = P_Max;					//比例最大值
    PID_Config->IMax = I_Max;					//积分最大值
	PID_Config->DMax = D_Max;					//微分最大值
}
/// @brief PID的零点处理初始化
/// @param PID_Config PID配置结构体
/// @param AngleMax 角度最大值[-AngleMax,,AngleMax]
void PID_Zero_Processing_Init(PIDConfig_t *PID_Config,PIDElem_t AngleMax)
{
	PID_Config->AngleMax = AngleMax;          //角度配置
}

/// @brief 基础位置式PID控制器
/// @param PIDConfig PID配置结构体
/// @param E_value 期望值
/// @param C_value 当前值
/// @return 当前PID输出
__attribute__((section("ccmram"))) PIDElem_t Basic_PID_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value)
{
	PIDElem_t CurrentValue = 0;
	PIDElem_t new_DiffValue = 0;
	//当前误差
	new_DiffValue = Constrain_PID(E_value - C_value,PIDConfig->PMax,-PIDConfig->PMax);
	
	//误差变量率(即误差的导数)
	PIDConfig->MissValue = Constrain_PID(new_DiffValue - PIDConfig->DiffValue,PIDConfig->DMax,-PIDConfig->DMax);
	PIDConfig->DiffValue = new_DiffValue;

	//积分求和
	PIDConfig->IntegralValue += PIDConfig->DiffValue;
	
	//抗积分饱和
	PIDConfig->IntegralValue = Constrain_PID(PIDConfig->IntegralValue,PIDConfig->IMax,-PIDConfig->IMax);

	//PID
	CurrentValue = Mul(PIDConfig->KP,PIDConfig->DiffValue) + Mul(PIDConfig->KI,PIDConfig->IntegralValue) + Mul(PIDConfig->KD,PIDConfig->MissValue);

	//抗PID饱和
	return Constrain_PID(CurrentValue,PIDConfig->PIDMax,PIDConfig->PIDMin);
}

/// @brief 基础位置式PI控制器
/// @param PIDConfig PI配置结构体
/// @param E_value 期望值
/// @param C_value 当前值
/// @return 当前PID输出
__attribute__((section("ccmram"))) PIDElem_t Basic_PI_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value)
{
	PIDElem_t CurrentValue = 0;
	PIDElem_t new_DiffValue = 0;
	//检查KD是否为0，防止使用PID控制器时错误调用PI控制器
	assert_param(PIDConfig->KD == 0);
	//当前误差
	PIDConfig->DiffValue = Constrain_PID(E_value - C_value,PIDConfig->PMax,-PIDConfig->PMax);
	
	//积分求和
	PIDConfig->IntegralValue += PIDConfig->DiffValue;
	
	//抗积分饱和
	PIDConfig->IntegralValue = Constrain_PID(PIDConfig->IntegralValue,PIDConfig->IMax,-PIDConfig->IMax);

	//PID
	CurrentValue = Mul(PIDConfig->KP,PIDConfig->DiffValue) + Mul(PIDConfig->KI,PIDConfig->IntegralValue);

	//抗PID饱和
	return Constrain_PID(CurrentValue,PIDConfig->PIDMax,PIDConfig->PIDMin);
}


/// @brief 带零点处理的PID控制器
/// @param PIDConfig PID配置结构体
/// @param E_value 期望值
/// @param C_value 当前值
/// @return 当前PID输出
/// @attention 零点处理是范围为[0,AngleMax],若范围是[-AngleMax,AngleMax]需要加上AngleMax/2
__attribute__((section("ccmram"))) PIDElem_t Zero_Processing_PID_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value)
{
	//角度突变点处理
	//机械角度超过半数则需要往反方向调节，此时要消除突变点
	if (abs(C_value - E_value) > (PIDConfig->AngleMax/2))
	{
		E_value += (E_value > C_value) ? -PIDConfig->AngleMax : PIDConfig->AngleMax;
	}
	return Basic_PID_Controller(PIDConfig,E_value, C_value);
}
__attribute__((section("ccmram"))) /// @brief 重置PID内部变量
/// @param PIDConfig PID配置
void PID_Reset(PIDConfig_t* PIDConfig)
{
	PIDConfig->DiffValue = 0;
	PIDConfig->IntegralValue = 0;
	PIDConfig->MissValue = 0;
}
/// @brief PID限幅配置
/// @param PIDConfig PID配置结构体
/// @param PID_Constrain PID限幅选择
/// @param new_max 最大值
/// @param new_min 最小值(只有PID输出限幅使用)
void PID_Constrain_Config(PIDConfig_t* PIDConfig,PID_Constrain_Config_t PID_Constrain,PIDElem_t new_max,PIDElem_t new_min)
{
	switch (PID_Constrain)
	{
	case PID_P_Constrain:
		PIDConfig->PMax = new_max;
		break;
	case PID_I_Constrain:
		PIDConfig->IMax = new_max;
		break;
	case PID_D_Constrain:
		PIDConfig->DMax = new_max;
		break;
	case PID_PID_Constrain:
		PIDConfig->PIDMax = new_max;
		PIDConfig->PIDMin = new_min;
		break;
	}
}
#ifdef TEMP_MUL
#undef Mul
#undef TEMP_MUL
#endif