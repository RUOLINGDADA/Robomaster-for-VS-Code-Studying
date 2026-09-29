#ifndef _CAP_H_
#define _CAP_H_
#include "main.h"
typedef struct 
{
  //均压时电荷不守恒
  struct 
  {
    float cap_coulomb_value;   // 电容电荷量
    float cap_energy_value;    // 电容目前能量
    float cap_energy_max;      // 电容总能量
  } energy_data;
  //c = i/du/dt
  //r = q/\int idt
  struct
  {
    float cap_value;           // 电容量
    float cap_resistance;      // 电容内阻
  } parameter_data;
  
} CAP_data_t;

extern CAP_data_t Cap_data;

void Coulomb_Calculation(float *coulomb_value, float I_value, float samp_time);
float Cap_Value_Calculation(float coulomb_value, float Vcap);
float Cap_Energy_Calculation(float *energy_max, float Vcap, float coulomb_value);
#endif