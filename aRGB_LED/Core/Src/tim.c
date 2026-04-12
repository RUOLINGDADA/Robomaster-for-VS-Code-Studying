/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include <stdint.h>

/* USER CODE BEGIN 0 */
// 浮点型定义（兼容大疆代码）
typedef float fp32;

// 宏定义：颜色切换时间 500ms
#define RGB_FLOW_COLOR_CHANGE_TIME  500

// 颜色数组：蓝 → 绿 → 红 → 蓝（长度4，实现无限循环）
// 格式：0x00RRGGBB （和你的aRGB_LED_Show函数完全匹配）
uint32_t RGB_flow_color[4] =
{
    0x000000FF,  // 蓝色
    0x0000FF00,  // 绿色
    0x00FF0000,  // 红色
    0x000000FF   // 回到蓝色（循环用）
};
/* USER CODE END 0 */

TIM_HandleTypeDef htim5;

/* TIM5 init function */
void MX_TIM5_Init(void)
{

    /* USER CODE BEGIN TIM5_Init 0 */

    /* USER CODE END TIM5_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM5_Init 1 */

    /* USER CODE END TIM5_Init 1 */
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 0;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 254;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM5_Init 2 */

    /* USER CODE END TIM5_Init 2 */
    HAL_TIM_MspPostInit(&htim5);

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

    if (tim_baseHandle->Instance == TIM5)
    {
        /* USER CODE BEGIN TIM5_MspInit 0 */

        /* USER CODE END TIM5_MspInit 0 */
        /* TIM5 clock enable */
        __HAL_RCC_TIM5_CLK_ENABLE();
        /* USER CODE BEGIN TIM5_MspInit 1 */

        /* USER CODE END TIM5_MspInit 1 */
    }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (timHandle->Instance == TIM5)
    {
        /* USER CODE BEGIN TIM5_MspPostInit 0 */

        /* USER CODE END TIM5_MspPostInit 0 */

        __HAL_RCC_GPIOH_CLK_ENABLE();
        /**TIM5 GPIO Configuration
        PH12     ------> TIM5_CH3
        PH11     ------> TIM5_CH2
        PH10     ------> TIM5_CH1
        */
        GPIO_InitStruct.Pin = LED_R_PWM_Pin | LED_G_PWM_Pin | LED_B_PWM_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM5_MspPostInit 1 */

        /* USER CODE END TIM5_MspPostInit 1 */
    }

}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

    if (tim_baseHandle->Instance == TIM5)
    {
        /* USER CODE BEGIN TIM5_MspDeInit 0 */

        /* USER CODE END TIM5_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM5_CLK_DISABLE();
        /* USER CODE BEGIN TIM5_MspDeInit 1 */

        /* USER CODE END TIM5_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
/*
  原数据：1
  跟 1 做 & → 1 & 1 = 1（保留）
  跟 0 做 & → 1 & 0 = 0（清零）
  原数据：0
  跟 1 做 & → 0 & 1 = 0（保留）
  跟 0 做 & → 0 & 0 = 0（清零）

  ARR = 254
  传入三十二位的数值分别对应每8位：
  Alpha Red Green Blue
*/
void aRGB_LED_Show(uint32_t RGBValue)
{
    // 扣每个颜色对应的数值并移到最低位
    uint32_t Red = (RGBValue & 0x00FF0000) >> 16; // 24 - 8
    uint32_t Green = (RGBValue & 0x0000FF00) >> 8;
    uint32_t Blue = RGBValue & 0x000000FF;
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_1, Blue);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_2, Green);
    __HAL_TIM_SetCompare(&htim5, TIM_CHANNEL_3, Red);
}
void fadeRGB(void)
{
    for (uint8_t j = 0; j < 255; j++)
    {
        aRGB_LED_Show((uint32_t)j << 16);
        HAL_Delay(5);  // 5ms延时，渐变平滑
    }
    for (uint8_t j = 255; j > 0; j--)
    {
        aRGB_LED_Show((uint32_t)j << 16);
        HAL_Delay(5);
    }

    // 2. 绿色呼吸
    for (uint8_t j = 0; j < 255; j++)
    {
        aRGB_LED_Show((uint32_t)j << 8);
        HAL_Delay(5);
    }
    for (uint8_t j = 255; j > 0; j--)
    {
        aRGB_LED_Show((uint32_t)j << 8);
        HAL_Delay(5);
    }

    // 3. 蓝色呼吸
    for (uint8_t j = 0; j < 255; j++)
    {
        aRGB_LED_Show((uint32_t)j);
        HAL_Delay(5);
    }
    for (uint8_t j = 255; j > 0; j--)
    {
        aRGB_LED_Show((uint32_t)j);
        HAL_Delay(5);
    }
}

/**
 * @brief  单颜色正弦波呼吸（自然柔和）
 * @param  shift: 颜色移位(16=红,8=绿,0=蓝)
 */
void Breath_LED_Smooth(uint8_t shift)
{
    // 0~180度正弦波，对应亮度0~255
    for (int i = 0; i <= 180; i++)
    {
        // 正弦计算亮度 (自动取0~255)
        uint8_t brightness = (sin(i * 3.1415926 / 180.0)) * 255;
        aRGB_LED_Show((uint32_t)brightness << shift);
        HAL_Delay(10);
    }
}

void RGB_Flow_Fade(void)
{
    uint8_t i;
    uint16_t j;
    fp32 delta_red, delta_green, delta_blue;  // 每ms的变化量
    fp32 red, green, blue;                   // 当前颜色值（浮点，保证平滑）
    uint32_t aRGB;                           // 最终输出颜色

    // 无限循环：遍历颜色表 0→1→2→3(0)
    for (i = 0; i < 3; i++)
    {
        // ============== 第一步：提取当前颜色 ==============
        red   = (RGB_flow_color[i] & 0x00FF0000) >> 16;
        green = (RGB_flow_color[i] & 0x0000FF00) >> 8;
        blue  = (RGB_flow_color[i] & 0x000000FF) >> 0;

        // ============== 第二步：计算每ms的变化增量 ==============
        // 下一个颜色 - 当前颜色 = 总变化量
        delta_red   = (fp32)((RGB_flow_color[i + 1] & 0x00FF0000) >> 16) - red;
        delta_green = (fp32)((RGB_flow_color[i + 1] & 0x0000FF00) >> 8)  - green;
        delta_blue  = (fp32)((RGB_flow_color[i + 1] & 0x000000FF) >> 0)  - blue;

        // 总变化量 / 500ms = 每1ms的微小增量
        /* 总变化量：
        红：0→0 不用变
        绿：0→255，一共要加 255
        蓝：255→0，一共要减 255 */
        delta_red   /= RGB_FLOW_COLOR_CHANGE_TIME;
        delta_green /= RGB_FLOW_COLOR_CHANGE_TIME;
        delta_blue  /= RGB_FLOW_COLOR_CHANGE_TIME;

        // ============== 第三步：500ms平滑渐变 ==============
        for (j = 0; j < RGB_FLOW_COLOR_CHANGE_TIME; j++)
        {
            // 每1ms累加微小增量
            red   += delta_red;
            green += delta_green;
            blue  += delta_blue;

            //     拼接成32位颜色
            aRGB = ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;

            // 输出到PWM灯
            aRGB_LED_Show(aRGB);

            // 1ms延时（500次刚好500ms）
            HAL_Delay(1);
        }
    }
}
/* USER CODE END 1 */

