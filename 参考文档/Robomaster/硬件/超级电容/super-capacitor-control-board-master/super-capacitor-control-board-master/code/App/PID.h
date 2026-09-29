#ifndef __PID_H_
#define __PID_H_

#include "mathfun.h"
typedef float PIDElem_t;				//PID元素类型
#define Constrain_PID(amt,high,low)		Constrain_float(amt,high,low) //PID限幅
typedef struct {
	PIDElem_t KP;						//比例因子
	PIDElem_t KI;						//积分因子
	PIDElem_t KD;						//微分因子
	PIDElem_t PMax;						//积分最大值
	PIDElem_t IMax;						//积分最大值
	PIDElem_t DMax;						//微分最大值
	PIDElem_t PIDMax;					//PID最大值
	PIDElem_t PIDMin;					//PID最大值
	PIDElem_t IntegralValue;			//积分
	PIDElem_t DiffValue;				//误差
	PIDElem_t MissValue;				//误差变化率
	PIDElem_t AngleMax;					//角度最大值
} PIDConfig_t;

typedef enum 
{
	PID_P_Constrain = 0,
	PID_I_Constrain,
	PID_D_Constrain,
	PID_PID_Constrain
} PID_Constrain_Config_t;

void PID_Init(PIDConfig_t *PID_Config,PIDElem_t K_P,PIDElem_t K_I,PIDElem_t K_D,PIDElem_t P_Max,PIDElem_t I_Max,PIDElem_t D_Max,PIDElem_t PID_Max,PIDElem_t PID_Min);
void PID_Zero_Processing_Init(PIDConfig_t *PID_Config,PIDElem_t AngleMax);

PIDElem_t Basic_PID_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value);
PIDElem_t Basic_PI_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value);
PIDElem_t Zero_Processing_PID_Controller(PIDConfig_t* PIDConfig,PIDElem_t E_value, PIDElem_t C_value);

void PID_Constrain_Config(PIDConfig_t* PIDConfig,PID_Constrain_Config_t PID_Constrain,PIDElem_t new_max,PIDElem_t new_min);
void PID_Integral_Config(PIDConfig_t *PID_Config,PIDElem_t integral_value);
void PID_Config(PIDConfig_t *PID_Config,PIDElem_t K_P,PIDElem_t K_I,PIDElem_t K_D,PIDElem_t P_Max,PIDElem_t I_Max,PIDElem_t D_Max);
void PID_Reset(PIDConfig_t* PIDConfig);
#endif
