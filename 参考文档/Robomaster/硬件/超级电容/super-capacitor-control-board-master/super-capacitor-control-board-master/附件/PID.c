#include "PID.h"
#include "math_filter.h"

/**
 * @brief PID配置初始化
 *     - 只配置最基础的pid参数
 *     - 抗积分饱和方式：积分遇限削弱法
 *     - 微分低通滤波:不滤波
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] Kp 比例因子
 * @param[in] Ki 积分因子
 * @param[in] Kd 微分因子
 * @param[in] PID_Max 输出最大限幅值
 * @param[in] PID_Min 输出最小限幅值
 * @param[in] ErrConstrain 误差限幅值
 * @param[in] sample_time 采样时间(秒)
 */
void PID_Init(PIDConfig_t *PID_Config, PIDElem_t Kp, PIDElem_t Ki, PIDElem_t Kd, PIDElem_t PID_Max, PIDElem_t PID_Min, PIDElem_t ErrConstrain, float sample_time)
{
	assert_param(sample_time > 0);
	assert_param(PID_Config != NULL);
	assert_param(PID_Max >= PID_Min);
	assert_param(ErrConstrain >= 0);

	PID_Config->Param.Kp = Kp;
	PID_Config->Param.Ki = Ki * sample_time;
	PID_Config->Param.Kd = Kd / sample_time;
	PID_Config->PIDMax = PID_Max;
	PID_Config->PIDMin = PID_Min;
	PID_Config->SampTime = sample_time;
	PID_Config->ErrConstrain = ErrConstrain;
	
	PID_Config->ITerm = 0;
	PID_Config->lest_ErrValue = 0;
	// 积分抗饱和方式：积分遇限削弱法
	PID_Config->anti_windup = anti_windup_clamping;
	// 默认抗积分饱和系数:ki/kp
	PID_Config->Ki_Clamp = PID_Config->Param.Ki/PID_Config->Param.Kp;
	// 默认微分低通滤波系数:1 (不滤波)
	PID_Config->Kd_lowpass = 1;
	// 默认不使用回调
	PID_Config->Callback_Fun = NULL;
	
}

/**
 * @brief PID优化配置
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] Ki_Clamp 抗积分饱和系数
 * @param[in] Kd_lowpass 微分低通滤波系数
 * @param[in] PID_Callback_Fun pid参数处理回调函数
 * @param[in] anti_windup 积分抗饱和方式
 */
void PID_Config(PIDConfig_t *PID_Config, PIDElem_t Ki_Clamp, const PIDElem_t Kd_lowpass, const PID_Callback_Fun_t PID_Callback_Fun, const PID_anti_windup_t anti_windup)
{
	assert_param(PID_Config != NULL);
	assert_param(Kd_lowpass >= 0 && Kd_lowpass <= 1);
	assert_param(Ki_Clamp >= 0);
	assert_param(IS_PID_ANTI_WINDUP(anti_windup));
	
	PID_Config->Ki_Clamp = Ki_Clamp;
	PID_Config->Kd_lowpass = Kd_lowpass;
	PID_Config->Callback_Fun = PID_Callback_Fun;
}

/**
 * @brief PID重置
 * @param[out] PID_Config PID配置结构体指针
 * @param[in] ITerm 积分值(乘以ki之后的)
 * @param[in] lest_ErrValue 上一次误差值
 */
void PID_Reset(PIDConfig_t *PID_Config,const PIDElem_t ITerm,const PIDElem_t lest_ErrValue)
{
	assert_param(PID_Config != NULL);

	PID_Config->ITerm = ITerm;
	PID_Config->lest_ErrValue = lest_ErrValue;
}

// /**
// 	@brief PID的零点处理初始化
// 	@param PID_Config PID配置
// 	@param AngleMax 角度最大值[-AngleMax,AngleMax]
// */	
// void Zero_Processing_PID_Init(PIDConfig_t *PID_Config,PIDElem_t AngleMax)
// {
// 	PID_Config->AngleMax = AngleMax;          //角度配置
// }
// /**
// 	@brief 带零点处理的PID控制器
// 	@param PIDConfig PID配置
// 	@param E_value 期望值
// 	@param C_value 当前值
// 	@return 当前PID输出
// 	@attention 零点处理是范围为[0,AngleMax],若范围是[-AngleMax,AngleMax]需要在输入值需要加上AngleMax以转换成[0,2AngleMax]
// */	
// PIDElem_t Zero_Processing_PID_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value)
// {
// 	//角度突变点处理
// 	//机械角度超过半数则需要往反方向调节，此时要消除突变点
// 	if (abs(C_value - E_value) > (PIDConfig->AngleMax/2))
// 	{
// 		E_value += (E_value > C_value) ? -PIDConfig->AngleMax : PIDConfig->AngleMax;
// 	}
// 	return Basic_PID_Controller(PIDConfig,E_value, C_value);
// }

/**
 * @brief 基础位置式PID控制器
 * @param[in,out] PIDConfig PID配置
 * @param[in] E_value 期望值
 * @param[in] C_value 实际值
 * @return 当前PID输出
 */
PIDElem_t Basic_PID_Controller(PIDConfig_t *PIDConfig, const PIDElem_t E_value, const PIDElem_t C_value)
{
	PIDElem_t PIDtemp,Itemp,Errtemp;
	assert_param(PID_Config != NULL);
	
	// 当前误差
	Errtemp = E_value - C_value;
	PIDConfig->ErrValue = Constrain_PID(Errtemp,PIDConfig->ErrConstrain,-PIDConfig->ErrConstrain);
	// 计算积分微分
	if(PIDConfig->Callback_Fun == NULL)
	{
		Itemp = PIDConfig->ErrValue * PIDConfig->Param.Ki;
		// 积分
		PIDConfig->ITerm += Itemp;
		// 微分
		PIDConfig->DiffValue = Lowpass((PIDConfig->ErrValue - PIDConfig->lest_ErrValue) *  PIDConfig->Param.Kd,PIDConfig->DiffValue,PIDConfig->Kd_lowpass);
		PIDConfig->lest_ErrValue = PIDConfig->ErrValue;
	}
	else
	{
		PIDConfig->Callback_Fun(PIDConfig,E_value,C_value);
	}
	// PID
	PIDtemp=PIDConfig->Param.Kp * PIDConfig->ErrValue + PIDConfig->DiffValue + PIDConfig->ITerm;
	// PID输出限制
	PIDConfig->PIDout=Constrain_PID(PIDtemp,PIDConfig->PIDMax,PIDConfig->PIDMin);
	// 抗积分饱和
	switch (PIDConfig->anti_windup)
	{
	case anti_windup_back_calculkation:
		PIDConfig->ITerm += (PIDConfig->PIDout-PIDtemp)*PIDConfig->Ki_Clamp;
		break;
	case anti_windup_clamping:
		if(PIDtemp != PIDConfig->PIDout && sign(PIDConfig->ErrValue)*sign(PIDtemp)>0)
		{
			PIDConfig->ITerm -= Itemp;
		}
		break;
	}

	return PIDConfig->PIDout;
}

/**
 * @brief 基础位置式PI控制器
 * @param[in,out] PIDConfig PID配置
 * @param[in] E_value 期望值
 * @param[in] C_value 实际值
 * @return 当前PID输出
 */
PIDElem_t Basic_PI_Controller(PIDConfig_t *PIDConfig, const PIDElem_t E_value, const PIDElem_t C_value)
{
	PIDElem_t PIDtemp,Itemp,Errtemp;
	assert_param(PID_Config != NULL);
	assert_param(PIDConfig->Param.Kd == 0);

	// 当前误差
	Errtemp = E_value - C_value;
	PIDConfig->ErrValue = Constrain_PID(Errtemp,PIDConfig->ErrConstrain,-PIDConfig->ErrConstrain);

	if(PIDConfig->Callback_Fun == NULL)
	{
		Itemp = PIDConfig->ErrValue * PIDConfig->Param.Ki;
		// 积分
		PIDConfig->ITerm += Itemp;
	}
	else
	{
		PIDConfig->Callback_Fun(PIDConfig,E_value,C_value);
	}
	// PID
	PIDtemp=PIDConfig->Param.Kp * PIDConfig->ErrValue + PIDConfig->ITerm;
	// PID输出限制
	PIDConfig->PIDout=Constrain_PID(PIDtemp,PIDConfig->PIDMax,PIDConfig->PIDMin);
	// 抗积分饱和
	switch (PIDConfig->anti_windup)
	{
	case anti_windup_back_calculkation:
		PIDConfig->ITerm += (PIDConfig->PIDout-PIDtemp)*PIDConfig->Ki_Clamp;
		break;
	case anti_windup_clamping:
		if(PIDtemp != PIDConfig->PIDout && sign(PIDConfig->ErrValue)*sign(PIDtemp)>0)
		{
			PIDConfig->ITerm -= Itemp;
		}
		break;
	}

	return PIDConfig->PIDout;
}