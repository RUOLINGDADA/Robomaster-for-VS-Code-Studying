#ifndef _CHASSIS_POWERLIMIT_H__
#define _CHASSIS_POWERLIMIT_H__

//底盘驱动轮个数
#define CHASSIS_DRIVE_NUMBER 4
#define CHASSIS_DRIVE_IMAX 12000.0f
/* 裁判系统接口 */
// 裁判系统底盘功率
#define Chassis_Power_Value             power_heat_data_t.chassis_power
// 裁判系统缓冲功率
#define Chassis_Power_Buffer_Value      power_heat_data_t.chassis_power_buffer
// 底盘限制功率
#define Chassis_Power_Limit_Value       game_robot_status_t.chassis_power_limit
// 底盘输出开关
#define Chassis_Power_Output            game_robot_status_t.mains_power_chassis_output
/* 底盘接口 */
#define Chassis_Set_Current(i)          chassis.motor[i].Current
/* 接口函数 */
void Chassis_Power_Limit(FunctionalState *Supercap_Mode);
#endif