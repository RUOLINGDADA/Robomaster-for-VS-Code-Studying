#include "POWER.h"
#include "sample.h"
#include "cap.h"

__attribute__((section("ccmram"))) float MosA_Duty, MosB_Duty, Samp_Dupy;
__attribute__((section("ccmram"))) Power_Loop_t Power;
__attribute__((section("ccmram"))) Power_Limit_Loop_t Power_limit;

/// @brief 电源初始化
/// @param
void Init_Power(void)
{
	Power.data.expect_cup_V = 0;
	Power.data.expect_out_V = 0;
	Power.data.boost_max_duty = BOOST_DUTY_MIN;
	Power.data.boost_min_duty = BOOST_DUTY_MIN;
	Power.data.buck_max_duty = BUCK_DUTY_MIN;
	Power.data.buck_min_duty = BUCK_DUTY_MIN;
	Power.data.setting_cup_V = CUP_V_MAX;
	Power.data.setting_out_V = 27;

	PID_Init(&Power.boost_I_PID, BOOST_I_P, BOOST_I_I, BOOST_I_D, BOOST_I_PMAX, BOOST_I_IMAX, BOOST_I_DMAX, 1, 0);
	PID_Init(&Power.buck_I_PID, BUCK_I_P, BUCK_I_I, BUCK_I_D, BUCK_I_PMAX, BUCK_I_IMAX, BUCK_I_DMAX, 1, 0);
	PID_Init(&Power.boost_V_PID, BOOST_V_P, BOOST_V_I, BOOST_V_D, BOOST_V_PMAX, BOOST_V_IMAX, BOOST_V_DMAX, BOOST_V_PIDMAX, 0);
	PID_Init(&Power.buck_V_PID, BUCK_V_P, BUCK_V_I, BUCK_V_D, BUCK_V_PMAX, BUCK_V_IMAX, BUCK_V_DMAX, BUCK_V_PIDMAX, 0);
	Power.status.value = 0x00;
	Power.error_status.value = 0x00;
	Power.power_mode = Standby;
	Power.running_status = Wait;

	Cap_data.energy_data.cap_coulomb_value = 0;
	Cap_data.energy_data.cap_energy_max = 100;
	Cap_data.energy_data.cap_energy_value = 0;
}

/// @brief 功率限制初始化
/// @param
void Init_Power_Limit(void)
{
	PID_Init(&Power_limit.Power_PID, POWER_P, POWER_I, POWER_D, POWER_PMAX, POWER_IMAX, POWER_DMAX, POWER_PIDMAX, -POWER_PIDMAX);
	PID_Init(&Power_limit.Power_Buffer_PID, POWER_BUFFER_P, POWER_BUFFER_I, POWER_BUFFER_D, POWER_BUFFER_PMAX, POWER_BUFFER_IMAX, POWER_BUFFER_DMAX, 20, -70);
	Power_limit.Power_PID.IntegralValue = 0;
	Power_limit.data.Pcup = 0;
	Power_limit.data.Pin = 0;
	Power_limit.data.Pout = 0;
	Power_limit.data.Pset = 50.0f;
	Power_limit.timestamp.rx_timestamp = 0;
	Power_limit.timestamp.compensate_timestamp = 0;
	Power_limit.config_data.Icharge_max = 14.0f;
	Power_limit.config_data.Icompensate_max = 16.0f;
}

/// @brief 电源环路
/// @attention 电源环路执行频率为50kHz
/// @param
__attribute__((section("ccmram"))) void Power_Loop(void)
{
	static PIDElem_t PIDoutput;
	static PIDElem_t expect_i;
	static PIDElem_t real_i;
	real_i = Icap.ave_value;
	// 电容电量计算
	Coulomb_Calculation(&Cap_data.energy_data.cap_coulomb_value, real_i, 1 / 50000.0f);

	switch (Power.power_mode)
	{
	case Boost:
		// 电压外环
		expect_i = Basic_PID_Controller(&Power.boost_V_PID, Power.data.expect_out_V, Vout.now_value);
		// 功率环限流
		expect_i = min(expect_i, Power.data.setting_max_I);
		// expect_i = 1;
		// 电流内环
		PIDoutput = Basic_PID_Controller(&Power.boost_I_PID, expect_i, -real_i);
		// PIDoutput = 0.5;
		MosB_Duty = CONDUCTION_DUTY;
		// 占空比限幅
		MosA_Duty = 1 - Constrain_float(PIDoutput, Power.data.boost_max_duty, Power.data.boost_min_duty);
		// 转换成pwm
		Set_HRTIMA(Floor(MosA_Duty * PERIOD));
		Set_HRTIMB(Floor(MosB_Duty * PERIOD));
		Set_Sample(Floor(MosA_Duty * PERIOD) / 2);
		break;
	case Buck:
		// 电压外环
		expect_i = Basic_PID_Controller(&Power.buck_V_PID, Power.data.expect_cup_V, Vcap.now_value);
		// 功率环限流
		expect_i = min(expect_i, Power.data.setting_max_I);
		// expect_i = 2.5;
		// 电流内环
		PIDoutput = Basic_PID_Controller(&Power.buck_I_PID, expect_i, real_i);
		MosB_Duty = CONDUCTION_DUTY;
		// 占空比限幅
		MosA_Duty = Constrain_float(PIDoutput, Power.data.buck_max_duty, Power.data.buck_min_duty);
		// 转换成pwm
		Set_HRTIMA(Floor(MosA_Duty * PERIOD));
		Set_HRTIMB(Floor(MosB_Duty * PERIOD));
		Set_Sample(Floor(MosA_Duty * PERIOD) / 2);
		break;
	case Standby:
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
		break;
	}
}

/// @brief 功率限制环路
/// @note 此处执行频率为200Hz
/// @param
__attribute__((section("ccmram"))) void Power_Limit_Loop(void)
{
	static float disturbance_power;
	static float temp, limit_expect_i;
	// 获取功率
	Power_limit.data.Pin = LowPass(Vin.ave_value * Iin.ave_value, Power_limit.data.Pin, PIN_LOWPASS_A);
	Power_limit.data.Pout = LowPass(Vout.ave_value * Iout.ave_value, Power_limit.data.Pout, POUT_LOWPASS_A);
	// Power_limit.Pcup = LowPass(Vcup.ave_value * Icup.ave_value,Power_limit.Pcup,PCUP_LOWPASS_A);
#if USE_POWER_LIMIT
	if (Power.power_mode != Standby)
	{
		// 功率缓冲环
		if (Power.status.flag.CanLossOfConnection == DISABLE)
		{
			disturbance_power = Basic_PID_Controller(&Power_limit.Power_Buffer_PID, EXPECT_POWER_BUFFER, Power_limit.data.power_buffer);
		}
		else // 2s未接收到数据报文认为是失联
		{
			disturbance_power = -Power_limit.data.Pset * 0.05f;
		}
		// 功率环计算期望电流，使用电容最大电流进行限幅
		temp = Basic_PID_Controller(&Power_limit.Power_PID, Power_limit.data.Pset + disturbance_power, Power_limit.data.Pin);
		limit_expect_i = temp * Vin.ave_value / Vcap.ave_value;

		// 根据输出进行限幅
		limit_expect_i = Constrain_float(temp, Power_limit.config_data.Icharge_max, -Power_limit.config_data.Icompensate_max);
		Power_limit.Power_PID.IntegralValue = Constrain_float(Power_limit.Power_PID.IntegralValue, Power_limit.config_data.Icharge_max / Power_limit.Power_PID.KI, -Power_limit.config_data.Icompensate_max / Power_limit.Power_PID.KI);

		// 功率环路状态
		if (limit_expect_i > 0.1f)
		{
			// 退出补偿模式超过设定值且不是只补偿模式进入充电模式
			if (HAL_GetTick() - Power_limit.timestamp.compensate_timestamp > 600) // && Power_limit.status.flag.power_limit_mode != OnlyCompensateMode)
			{
				// 进入充电模式
				Power_limit.mode = cup_charge;
			}
			else
			{
				limit_expect_i = 0;
				Power_limit.Power_PID.IntegralValue = limit_expect_i / Power_limit.Power_PID.KI;
			}
		}
		else if (limit_expect_i < 0.1f)
		{
			// 不是只充电模式进入放电模式
			if (1) // Power_limit.status.flag.power_limit_mode != OnlyChangeMode)
			{
				// 进入功率补偿模式
				Power_limit.mode = cup_compensate;
				Power_limit.timestamp.compensate_timestamp = HAL_GetTick();
			}
			else
			{
				limit_expect_i = 10.0f / Vcap.ave_value;
				Power_limit.Power_PID.IntegralValue = limit_expect_i / Power_limit.Power_PID.KI;
			}
		}
		Power_limit.config_data.expect_i = limit_expect_i;
		switch (Power_limit.mode)
		{
		case cup_charge:
			// 进入充电模式
			// 电容没电时恒流充电
			if (Vcap.ave_value < 8.0f)
			{
				Power_limit.config_data.expect_i = Constrain_float(Power_limit.config_data.expect_i, 3.0f, -Power_limit.config_data.Icompensate_max);
				Power_limit.Power_PID.IntegralValue = Power_limit.config_data.expect_i / Power_limit.Power_PID.KI;
			}

			Power.data.setting_max_I = abs(Power_limit.config_data.expect_i);
			Power.data.setting_cup_V = CUP_V_MAX;
			LED_G(0);
			Power_Mode_Switch(Buck);
			break;

		case cup_compensate:
			// 进入功率补偿模式
			Power.data.setting_max_I = abs(Power_limit.config_data.expect_i);
			Power.data.setting_out_V = Vin.ave_value < 27.5 ? Vin.ave_value + 1 : 27.5;
			LED_G(1);
			Power_Mode_Switch(Boost);
#if USE_CUP_UNDERVOLTAGE_CLOSE
			// 电容低于关闭电压关闭功率环路
			if (Power.status.flag.CupUndervoltage == ENABLE)
			{
				Power.running_status = Wait;
			}
#endif
			if (Power_limit.config_data.Icompensate_max == 0)
			{
				Power.running_status = Wait;
			}
			break;
		}
#if USE_CUP_FULL_CLOSE
		if (Power.power_mode == Buck)
		{
			// 电容高于充电电压关闭功率环路
			if (Vcup.ave_value >= CUP_V_MAX)
			{
				Power.running_status = Wait;
			}
		}
#endif
	}
#else
	Power.data.setting_max_I = 5.0f;
#endif
}

/// @brief 电源状态控制
/// @param
__attribute__((section("ccmram"))) void Status_Control(void)
{
	if (Power.status.flag.CupEnable == DISABLE)
	{
		Power_Mode_Switch(Standby);
		LED_B(0);
		LED_G(0);
	}
	else if (Power.status.flag.CupEnable == ENABLE && Power.power_mode == Standby)
	{
		Power_Mode_Switch(Buck);
		LED_B(1);
	}
	if (Power.error_status.value != 0 || Power.status.flag.PowerLoopError == ENABLE)
	{
		BUZZER(1);
	}
	else
	{
		BUZZER(0);
	}
	if (HAL_GetTick() - Power_limit.timestamp.rx_timestamp < 20)
	{
		Power.status.flag.CanLossOfConnection = DISABLE;
	}
	else // 1s未接收到数据报文认为是失联
	{
		Power.status.flag.CanLossOfConnection = ENABLE;
	}
	Cap_data.energy_data.cap_energy_value = Cap_Energy_Calculation(&Cap_data.energy_data.cap_energy_max, Vcap.ave_value, Cap_data.energy_data.cap_coulomb_value);
	Cap_data.parameter_data.cap_value = Cap_Value_Calculation(Cap_data.energy_data.cap_coulomb_value, Vcap.ave_value);
}
/// @brief 电源环路异常捕获
/// @note 此处执行频率为200Hz
/// @param
__attribute__((section("ccmram"))) void Power_Error_Taip(void)
{
	static uint8_t Err_count = 0, looperr_count = 0; // 错误计数
	static bool Err;
	static uint32_t err_timestamp = 0;
	Err = false;
	// // 输出过压检测
	// if (Vout.now_value > 35.0f)
	// {
	// 	Err = true;
	// 	Err_count++;
	// 	if (Err_count > 1)
	// 	{
	// 		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
	// 		Power.error_status.flag.OutputOvervoltage = ENABLE;
	// 		Power.status.flag.PowerLoopError = ENABLE;
	// 		Power.running_status = Error;
	// 	}
	// }
	// else
	// {
	// 	Power.error_status.flag.OutputOvervoltage = DISABLE;
	// }

	// 	电容过压检测
	// 	if (Vcup.now_value > CUP_V_MAX + 1.0f)
	// 	{
	// 		Err = true;
	// 		Err_count++;
	// 		if (Err_count > 2)
	// 		{
	// 			// 过压立马关断电源环路
	// 			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
	// 			Power.error_status.flag.CupOvervoltage = ENABLE;
	// 			Power.status.flag.PowerLoopError = ENABLE;
	// 			Power.running_status = Error;
	// 			Err_timestamp = timestamp;
	// 		}
	// 	}
	// #if USE_ERROR_AUTO_RESTORE
	// 	else if (Power.error_status.flag.CupOvervoltage && timestamp - Err_timestamp > 20)
	// 	{
	// 		Power.error_status.flag.CupOvervoltage = DISABLE;
	// 		Power_Mode_Switch(Buck);
	// 	}
	// #endif
	//
	if ((Iin.ave_value - Iout.ave_value) > 1.5f && Icap.ave_value < 0.2f)
	{
		Err_count++;
		Err = true;
		if (Err_count > 100)
		{
			HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
			Power.error_status.flag.LoopOvercurrent = ENABLE;
			Power.running_status = Error;
			Err_count = 0;
			err_timestamp = HAL_GetTick();
			looperr_count++;
		}
	}
	else if (HAL_GetTick() - err_timestamp > 2000 && Power.error_status.value && !Power.status.flag.PowerLoopError)
	{
		Power.error_status.value = 0;
		Power.status.flag.PowerLoopError = DISABLE;
		Power.running_status = Wait;
		Power.power_mode = Standby;
	}
	if (looperr_count > 3)
	{
		Power.status.flag.PowerLoopError = ENABLE;
		looperr_count = 0;
	}
	// // 电容电压采样 1000ms采样一次
	// static float Vcup_last;
	// if (timestamp % 10 == 0)
	// {
	// 	if (Vcup_last - Vcup.ave_value > 2.0f && Vcup.ave_value < ((CUP_V_CLOSE - 2.0f >= 3.0f) ? (CUP_V_CLOSE - 2.0f) : 3.0f))
	// 	{
	// 		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
	// 		Power.running_status = Error;
	// 		Power.status.flag.PowerLoopError = ENABLE;
	// 	}
	// 	Vcup_last = Vcup.ave_value;
	// }
	// 电容欠压检测
	if (Vcap.ave_value < CUP_V_CLOSE)
	{
		Err = true;
		Err_count++;
		if (Err_count >= 3)
		{
			Power.status.flag.CupUndervoltage = ENABLE;
		}
	}
	else if (Vcap.ave_value > 14.0f)
	{
		Power.status.flag.CupUndervoltage = DISABLE;
	}
	// 缓冲功率低
	// if (Power.status.flag.CanLossOfConnection == DISABLE)
	//{
	//	if (Power_limit.data.power_buffer < 5.0f && Power_limit.mode == cup_charge)
	//	{
	//		Power.error_status.flag.InsufficientPowerBuffer == ENABLE;
	//	}
	//	else if (Power.error_status.flag.InsufficientPowerBuffer == ENABLE && Power_limit.data.power_buffer > 30.0f)
	//	{
	//		Power.error_status.flag.InsufficientPowerBuffer == DISABLE;
	//	}
	//}
	// else
	//{
	//	Power.error_status.flag.InsufficientPowerBuffer == DISABLE;
	//}
	// // 超过5s仍然无法充电认为环路异常
	// static uint16_t power_loop_error_count = 0;
	// if (Power.power_mode == Buck && Iin.now_value + Iout.now_value > 0.5f && Vcup.ave_value < 2.0f)
	// {
	// 	power_loop_error_count++;
	// 	if (power_loop_error_count > 1000)
	// 	{
	// 		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
	// 		Power.status.flag.PowerLoopError = ENABLE;
	// 		Power.running_status = Error;
	// 	}
	// }
	// else
	// {
	// 	power_loop_error_count = 0;
	// }

	if (Err == false)
	{
		Err_count = 0;
	}
}
/// @brief 电源环路运行模式切换
/// @note 此处执行频率为200Hz
__attribute__((section("ccmram"))) void Power_Loop_Mode(void)
{
#if USE_POWER_LOOP
	// 等待状态下关闭环路
	if (Power.power_mode == Standby)
	{
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
		Power.running_status = Wait;
		return;
	}
	switch (Power.running_status)
	{
	case Init: // 初始化功率环路参数，跳转到软起动。
		float temp = Vcap.ave_value > Vout.ave_value ? Vout.ave_value / Vcap.ave_value : Vcap.ave_value / Vout.ave_value;
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
		PID_Reset(&Power.buck_I_PID);
		PID_Reset(&Power.buck_V_PID);

		Power.data.boost_min_duty = Constrain_float(1 - temp, BOOST_DUTY_MAX, BOOST_DUTY_MIN);
		// Power.data.boost_min_duty = BOOST_DUTY_MAX;
		Power.data.buck_max_duty = Constrain_float(temp, BUCK_DUTY_MAX, BUCK_DUTY_MIN);
		Power.data.boost_max_duty = BOOST_DUTY_MAX;
		Power.data.buck_min_duty = BUCK_DUTY_MIN;

		Power.data.expect_out_V = Vout.ave_value;
		Power.data.expect_cup_V = Vcap.ave_value;
		Power.running_status = Start;
		HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2); // 开启PWM输出和PWM计时器
		break;
	case Start:
		// 设定电压缓启动
		Power.data.expect_cup_V = Ramp_float(Power.data.setting_cup_V, Power.data.expect_cup_V, 0.1);
		Power.data.expect_out_V = Ramp_float(Power.data.setting_out_V, Power.data.expect_out_V, 0.1);
		// 占空比缓启动
		Power.data.boost_min_duty = Ramp_float(BOOST_DUTY_MIN, Power.data.boost_min_duty, 0.2);
		Power.data.buck_max_duty = Ramp_float(BUCK_DUTY_MAX, Power.data.buck_max_duty, 0.2);
		// 占空比缓启动完成则进入运行状态
		if (abs(Power.data.boost_min_duty - BOOST_DUTY_MIN) <= 0.05 && abs(Power.data.buck_max_duty - BUCK_DUTY_MAX) <= 0.05)
		{
			Power.data.boost_min_duty = BOOST_DUTY_MIN;
			Power.data.buck_max_duty = BUCK_DUTY_MAX;
			Power.running_status = Run;
		}
		break;
	case Run:
		// 期望电压缓慢跟随设定电压，减少超调
		Power.data.expect_cup_V = Ramp_float(Power.data.setting_cup_V, Power.data.expect_cup_V, 0.2);
		Power.data.expect_out_V = Ramp_float(Power.data.setting_out_V, Power.data.expect_out_V, 0.2);
		break;
	case Wait:
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
		break;
	case Error:
		HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
		break;
	}
#else
	Power.running_status = Wait;
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
#endif
}

/// @brief 电源模式切换
/// @param power_mode 电源模式
__attribute__((section("ccmram"))) void Power_Mode_Switch(uint32_t power_mode)
{
	assert_param((power_mode == Buck || power_mode == Boost || power_mode == Standby));
	// 切换模式与当前模式相同不切换
	if (power_mode == Power.power_mode && Power.running_status != Wait)
	{
		return;
	}
	// 出现错误不切换
	if (Power.running_status == Error)
	{
		return;
	}
	HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
	Power.running_status = Init;
	Power.power_mode = power_mode;
}

// /// @brief 电源采样标定
// /// @param ADC_config ADC数据结构体
// /// @return true:标定完成 false:标定未完成
// bool Power_Sampling_Calibration(ADC_Data_t *ADC_config,float* calibration_value)
// {
// 	static uint32_t ADC_hash[20] = {0};
// 	uint32_t ADC_code = (uint32_t)ADC_config % 20;
// 	bool hash_hit = false;
// 	static float ADC_sample_value[20][ADC_CALIBRATION_DATA_SIZE] = {0}; //ADC采样值
// 	static uint32_t ADC_sample_count[20] = {0};	 //ADC采样位置
// 	float ADC_calibration_K,ADC_calibration_B;	//ADC采样值类型
// 	switch (ADC_config->type)
// 	{
// 	case type_voltage:
// 		/* code */
// 		break;
// 	case type_current:

// 		break;
// 	default:
// 		break;
// 	}
// 	//遍历哈希表
// 	for (int i = 0; i < 20 % 3; i += 3)
// 	{
// 		if (ADC_hash[(ADC_code + i) % 20] == (uint32_t)ADC_config)
// 		{
// 			hash_hit == true;
// 			ADC_code = (ADC_code + i) % 20;
// 			break;
// 		}
// 	}
// 	if (hash_hit == false)
// 	{
// 		for (int i = 0; i < 20 % 3; i += 3)
// 		{
// 			// 此处哈希表为空时，添加到哈希表，并且返回当前采样值
// 			if (ADC_hash[(ADC_code + i) % 20] == 0)
// 			{
// 				ADC_hash[(ADC_code + i) % 20] = (uint32_t)ADC_config;
// 				ADC_code = (ADC_code + i) % 20;
// 				*calibration_value = ADC_calibration_K * ADC_sample_count[ADC_code] + ADC_calibration_B;
// 				return false;
// 			}
// 		}
// 	}

// 	//开始标定 求均值
// 	if(ADC_sample_count[ADC_code]++ < 100)
// 	{
// 		ADC_sample_value[ADC_code] += *(ADC_config->ADC_value);
// 		return false;
// 	}
// 	else
// 	{
// 		ADC_sample_value[ADC_code] /= 100;
// 		return true;
// 	}

// }
/***************************End of File**********************************/