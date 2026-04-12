#include "user_delay.h"
#include <stdint.h>

void user_delay_us(uint32_t us)
{
    for (; us > 0; us--)
    {
        for (volatile uint8_t i = 50; i > 0; i--)
        {
            ;
        }
    }
}

void nop_delay_us(uint32_t us)
{
    while (us--)
    {
        // volatile 禁止编译器优化
        // 168Mhz 一秒数168000000个数(tick)
        // 一微秒数 168Mhz / 1000000 个数
        volatile uint32_t i = (uint32_t)168 / 10U;
        // __NOP() 是空操作指令，1个NOP=1个时钟周期
        while (i--)
        {
            __NOP();
        }
    }
}

void nop_delay_ms(uint32_t ms)
{
    while (ms--)
    {
        volatile uint32_t i = (uint32_t)168000 / 10U;
        while (i--)
        {
            __NOP();
        }
    }
}

void delay_us(uint32_t us)
{
    // 获取系统时钟频率 (单位Hz)
    uint32_t sys_clk = SystemCoreClock;
    // 计算需要的SysTick计数次数: 1us = sys_clk / 1000000 个时钟周期
    uint32_t wait_ticks = us * (sys_clk / 1000000U);
    uint32_t start = SysTick->VAL;    // 记录当前计数值
    // 循环等待计数差值达到目标
    while ((start - SysTick->VAL) < wait_ticks);
}