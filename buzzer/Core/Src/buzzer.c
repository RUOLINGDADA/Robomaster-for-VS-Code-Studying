#include "buzzer.h"

uint16_t buzzer_arr = 20999; // 重载值
uint16_t buzzer_psc = 0;     // 预分频

// 蜂鸣器初始化 4kHz
void Buzzer_Init(void)
{
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, buzzer_arr / 2);
    __HAL_TIM_SET_PRESCALER(&htim4, buzzer_psc);
    __HAL_TIM_SET_AUTORELOAD(&htim4, buzzer_arr);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
}

// 设置频率函数
void Buzzer_SetFreq(uint16_t freq)
{
    uint32_t timer_clk = 84000000; // APB1 84MHz
    uint16_t arr = (timer_clk / freq) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim4, arr);
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, arr / 2); // 固定50%占空比
}
