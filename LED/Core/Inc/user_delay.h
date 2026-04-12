#ifndef USER_DELAY_H
#define USER_DELAY_H
#include "stm32f4xx.h"
/*
这些函数都有误差:
volatile 会让 CPU每次都去内存读 i、写 i（巨耗时）
while 循环的判断、跳转，编译后一堆汇编指令
真实耗时 = 理论值的 5~10 倍
*/
void user_delay_us(uint32_t us);
void nop_delay_us(uint32_t us);
void nop_delay_ms(uint32_t ms);
/*
SysTick->VAL 是24 位递减计数器
硬性上限：
最大计数值 = 2²⁴ - 1 = 16777215
最大传入  16777215 ÷ 168 ≈ 99864us
*/
void delay_us(uint32_t us);
#endif
