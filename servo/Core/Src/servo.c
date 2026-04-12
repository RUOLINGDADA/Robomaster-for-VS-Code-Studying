#include "servo.h"




#define INIT_PULSE 500

void Servo_Init(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 500);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void Servo_SetAngle(uint8_t angleValue)
{
    /* 0.5ms  1.0ms   1.5ms   2.0ms   2.5ms
        -90°    -45°    0°      45°     90°
       500                              2500
    */
    uint16_t pulse = (2500.0 - 500.0) / 180.0 * angleValue + 500.0;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}