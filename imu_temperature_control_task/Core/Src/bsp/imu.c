#include "imu.h"

void Set_IMU_PWM(uint32_t value) { TIM10->CCR1 = value; }