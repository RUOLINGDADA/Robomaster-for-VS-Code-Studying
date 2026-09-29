#include "DCDC.h"


/**
 * @brief 四开关buck-boost电源环路初始化
 * @param[out] Power_Config 电源环路配置指针
 * @param[in] max_transform_ratio 最大变换比
 * @param[in] conduction_duty 导通占空比
 * @param[in] control_frec 控制频率(Hz,赫兹)
 * @param[in] ILoop_Param 电流环pid参数
 * @param[in] VLoop_Param 电压环pid参数
 */
void BuckBoost_Init(BuckBoost_Config_t *Power_Config, float max_transform_ratio,float conduction_duty, float control_frec, PID_Param_t ILoop_Param, PID_Param_t VLoop_Param)
{
	assert_param(Power_Config != NULL);
	assert_param(control_frec != 0);
	assert_param(conduction_duty > 0 && conduction_duty <= 1);
	assert_param(max_transform_ratio > 0);
	
	Power_Config->config_data.conduction_duty = conduction_duty;
	Power_Config->config_data.boost_min_duty = 1.0f/max_transform_ratio;
	Power_Config->config_data.control_frec = control_frec;

	Power_Config->running_status = Wait;
	Power_Config->power_mode = Buck;

	Power_Config->data.buck_max_duty = 0;
	Power_Config->data.boost_min_duty = 0;

	PID_Init(&Power_Config->Iloop_PID,ILoop_Param.Kp,ILoop_Param.Ki,ILoop_Param.Kd,max_transform_ratio,0,10.0f,1.0f/control_frec);
	PID_Init(&Power_Config->Vloop_PID,VLoop_Param.Kp,VLoop_Param.Ki,VLoop_Param.Kd,max_transform_ratio,0,10.0f,1.0f/control_frec);

	Power_Config->setting_data.setting_max_I = 0;
	Power_Config->setting_data.setting_out_V = 0;
}

/**
 * @brief 四开关buck-boost参数配置
 * @param[out] Power_Config 电源环路配置指针
 * @param[in] Vset 设定电压(V,伏)
 * @param[in] Imax 最大电流(A,安)
 */
void BuckBoost_Config(BuckBoost_Config_t *Power_Config, float Vset, float Imax)
{
	assert_param(Power_Config != NULL);

	Power_Config->setting_data.setting_out_V = Vset;
	Power_Config->setting_data.setting_max_I = Imax;
}

/**
 * @brief 四开关buck-boost电源环路控制
 * @param[in,out] Power_Config 电源环路配置指针
 * @param[in] Vout 输出电压(V,伏)
 * @param[in] Iout 输出电流(A,安)
 * @param[out] buck_duty buck占空比(D,上管占空比)
 * @param[out] boost_duty boost占空比(1-D,上管占空比)
 * @attention 传入的输出电流需要以输出方向为正方向
 */
_FAST inline void BuckBoost_Loop_Control(BuckBoost_Config_t *Power_Config,const float Vout,const float Iout, float *buck_duty, float *boost_duty)
{
	assert_param(Power_Config != NULL);
	assert_param(buck_duty != NULL);
	assert_param(boost_duty != NULL);
	float PIDoutput;
	float Vloop_out = 0,Iloop_out = 0;
	if(Power_Config->running_status == Wait)
	{
		*buck_duty = 0;
		*boost_duty = 0;
		return;
	}
    // 电压环
	Vloop_out = Basic_PID_Controller(&Power_Config->Vloop_PID, Power_Config->data.expect_out_V, Vout);
	// 电流环
	Iloop_out = Basic_PI_Controller(&Power_Config->Iloop_PID, Power_Config->setting_data.setting_max_I, Iout);
	// 计算pid
	PIDoutput = min(Vloop_out, Iloop_out);


	//[0,0.8]:buck
	if (PIDoutput < 0.8f)
	{
		*boost_duty = Power_Config->config_data.conduction_duty;
		// 占空比限幅
		*buck_duty = constrain(PIDoutput, Power_Config->data.buck_max_duty, 0);
		Power_Config->power_mode = Buck;
	}
	//(0.8,1.25]:buck-boost
	else if (PIDoutput < 1.25f)
	{
		*boost_duty = constrain(0.444f * (1 + 1.0f / PIDoutput), Power_Config->config_data.conduction_duty, Power_Config->data.boost_min_duty);
		// 占空比限幅
		*buck_duty = constrain(0.444f * (1 + PIDoutput), Power_Config->data.buck_max_duty, 0);
		Power_Config->power_mode = Buck_Boost;
	}
	//(1.25,+inf]:boost
	else
	{
		*boost_duty = constrain(1 / PIDoutput, Power_Config->config_data.conduction_duty, Power_Config->data.boost_min_duty);
		// 占空比限幅
		*buck_duty = Power_Config->config_data.conduction_duty;
		Power_Config->power_mode = Boost;
	}
}

/**
 * @brief 四开关buck-boost状态控制
 * @param[in,out] Power_Config 电源环路配置
 * @param[in] Vin 输入电压(V,伏)
 * @param[in] Vout 输出电压(V,伏)
 * @param[in] Power_switch 电源开关状态
*/
_FAST void BuckBoost_Status_Control(BuckBoost_Config_t *Power_Config, float Vin, float Vout, FunctionalState Power_switch)
{
	assert_param(Power_Config != NULL);
	assert_param(Power_switch == ENABLE || Power_switch == DISABLE);
	// 切换开关状态
	if(Power_switch == (Power_Config->running_status == Wait ? ENABLE : DISABLE))
	{
		if(Power_switch == ENABLE)
		{
			Power_Config->running_status = Init;
		}
		else
		{
			Power_Config->running_status = Wait;
		}
	}
	switch (Power_Config->running_status)
	{
	case Init: // 初始化功率环路参数，跳转到软起动。
		float temp = Vout / Vin;
        
		PID_Reset(&Power_Config->Iloop_PID,0,0);
		PID_Reset(&Power_Config->Vloop_PID,Vout/Vin,0);
        
		Power_Config->data.boost_min_duty = constrain(1-temp, Power_Config->config_data.conduction_duty, Power_Config->config_data.boost_min_duty);
		Power_Config->data.buck_max_duty = constrain(temp, Power_Config->config_data.conduction_duty, 0);

		Power_Config->data.expect_out_V = Vout;
		Power_Config->running_status = Start;
		break;
	case Start:
		// 设定电压缓启动
		Power_Config->data.expect_out_V = Ramp_float(Power_Config->setting_data.setting_out_V, Power_Config->data.expect_out_V, 0.1);
		// 占空比缓启动
		Power_Config->data.boost_min_duty = Ramp_float(Power_Config->config_data.boost_min_duty, Power_Config->data.boost_min_duty, 0.07);
		Power_Config->data.buck_max_duty = Ramp_float(Power_Config->config_data.conduction_duty, Power_Config->data.buck_max_duty, 0.07);
		// 占空比缓启动完成则进入运行状态
		if (abs(Power_Config->data.boost_min_duty - Power_Config->config_data.boost_min_duty) <= 0.05 && abs(Power_Config->data.buck_max_duty - Power_Config->config_data.conduction_duty) <= 0.05)
		{
			Power_Config->data.boost_min_duty = Power_Config->config_data.boost_min_duty;
			Power_Config->data.buck_max_duty = Power_Config->config_data.conduction_duty;
			Power_Config->running_status = Run;
		}
		break;
	case Run:
		// 期望电压缓慢跟随设定电压，减少超调
		Power_Config->data.expect_out_V = Ramp_float(Power_Config->data.expect_out_V, Power_Config->setting_data.setting_out_V, 0.2);
		break;
	case Wait:
		break;
	case Error:
		break;
	default:
		assert_param(0);
	}
}

/**
 * @brief 最大功率点追踪初始化
 * @param[out] mppt_config 最大功率点配置结构体
 * @param[in] max_duty 最大占空比
 * @param[in] min_duty 最小占空比
 * @param[in] step_longth 每次迭代步长
 */
void mppt_Init(mppt_config_t *mppt_config,float max_duty,float min_duty, float step_longth)
{
	assert_param(mppt_config != NULL);
	assert_param(max_duty <= 1 && max_duty >= 0);
	assert_param(min_duty <= max_duty && min_duty >= 0);
	assert_param(step_longth < 1 && step_longth > 0);

	mppt_config->max_duty = max_duty;
	mppt_config->min_duty = min_duty;
	mppt_config->step_longth = step_longth;

	mppt_config->Pin = 0;
	mppt_config->lest_Pin = 0;
	mppt_config->mppt_duty = max_duty;
	mppt_config->mppt_direction = mppt_sub;
}

/**
 * @brief 最大功率点追踪
 * @param[in,out] mppt_config 最大功率点配置结构体
 * @param[in] Vin 输入电压
 * @param[in] Iin 输入电流
 * @return mppt占空比(上管占空比)
 */
float mppt(mppt_config_t *mppt_config,float Vin,float Iin)
{
	assert_param(mppt_config != NULL);

	mppt_config->lest_Pin = mppt_config->Pin;
	mppt_config->Pin = Vin*Iin;

	if(abs(mppt_config->Pin - mppt_config->lest_Pin) < 0.05f)
	{
		// 上一次功率误差和这一次误差小于则不操作，认为已经达到最大功率点。
	}
	else if(mppt_config->Pin>mppt_config->lest_Pin)
	{
		// 这一次功率误差小于上一次误差，往当前方向移动
		if(mppt_config->mppt_direction == mppt_add)
			mppt_config->mppt_duty += mppt_config->step_longth;
		else 
			mppt_config->mppt_duty -= mppt_config->step_longth;
	}
	else
	{
		// 这一次功率误差大于上一次误差，往反方向移动
		if(mppt_config->mppt_direction == mppt_add)
		{
			mppt_config->mppt_duty -= mppt_config->step_longth;
			mppt_config->mppt_direction = mppt_sub;
		}
		else 
		{
			mppt_config->mppt_duty += mppt_config->step_longth;
			mppt_config->mppt_direction = mppt_add;
		}
	}

	mppt_config->mppt_duty = constrain(mppt_config->mppt_duty,mppt_config->max_duty,mppt_config->min_duty);
	return mppt_config->mppt_duty;
}