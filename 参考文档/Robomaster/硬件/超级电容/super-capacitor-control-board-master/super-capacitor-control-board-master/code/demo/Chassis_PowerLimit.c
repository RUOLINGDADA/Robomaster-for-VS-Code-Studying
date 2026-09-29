#include "Chassis_PowerLimit.h"
#include "Supercap.h"
#include "PID.h"
#include "Task_Chassis.h"
#include "reference.h"
#define abs(a) (a) > 0 ? (a) : -(a)
float Chassis_pidout_max;

static void get_judge_data(float *power, float *buffer, uint16_t *powmax)
{
    *power = Chassis_Power_Value;
    *buffer = Chassis_Power_Buffer_Value;
    *powmax = Chassis_Power_Limit_Value;
}
static void Power_Limit_Constrain(float *x, float min, float max)
{
    if (*x > max)
    {
        *x = max;
    }
    else if (*x < min)
    {
        *x = min;
    }
}

void Chassis_Power_Limit(FunctionalState *Supercap_Mode)
{
    static float Klimit = 1;
    static float Blimit = 1;
    static float Plimit = 0;
    static float Tlimit = 1;
    static float power, power_buffer;
    static uint16_t judge_power_max;
    static float chassis_volt;
    static float k = 1;
    get_judge_data(&power, &power_buffer, &judge_power_max);

    // 缓冲能量占比环，总体约束
    if (power_buffer >= 55)
        Blimit = 1;
    else if (power_buffer >= 50)
        Blimit = 0.9; // 15
    else if (power_buffer >= 40)
        Blimit = 0.75;
    else if (power_buffer >= 35)
        Blimit = 0.5;
    else if (power_buffer >= 25)
        Blimit = 0.25;
    else if (power_buffer >= 15)
        Blimit = 0.125;
    else if (power_buffer >= 10)
        Blimit = 0.05;
    else if (power_buffer >= 5)
        Blimit = 0.002;
    else
        Blimit = 0.0001;
    // 功率限幅环
    Tlimit = 1;
    float temp , Im;
	if(power_buffer<20)
		Im = CHASSIS_DRIVE_IMAX * 0.3;
	else
		Im = CHASSIS_DRIVE_IMAX;
    for(int i = 0;i<CHASSIS_DRIVE_NUMBER;i++)
    {
        temp = abs(Im / Chassis_Set_Current(i));
        if(Tlimit>temp)
        {
            Tlimit = temp;
        }
    }
	
    // 功率环
    if (*Supercap_Mode == ENABLE)
    {	
        // 开启超级电容的矫正函数，配合缓冲环使用
        Plimit = 0.03 * power_buffer + 0.8;
        Set_SuperCap_CompensationLimit(200);
        if(power_buffer < 40)
        {
            Set_SuperCap_PowerLimit(judge_power_max * 0.4);
        }
        else
        {
            Set_SuperCap_PowerLimit(judge_power_max);
        }
        if(Get_SuperCap_Vcap() < 8.0f)
        {
            //欠压关闭超级电容模式
            *Supercap_Mode = DISABLE;
            Set_SuperCap_PowerLimit(0);
            Plimit = 1;
        }
    }
    else
    {
        //更新超级电容功率
        Set_SuperCap_PowerLimit(judge_power_max);
        Plimit = 1;
        // 关闭超级电容输出
        Set_SuperCap_CompensationLimit(0);
    }

    if (Chassis_Power_Output == DISABLE)
    {
        Plimit = 0;
    }
    Klimit = Blimit * Plimit * Tlimit;
	Power_Limit_Constrain(&Klimit,0.001,1);
    //等比缩放电流
    for(i=0;i<CHASSIS_DRIVE_NUMBER;i++)
    {
        Chassis_Set_Current(i) *= Klimit;
    }
}