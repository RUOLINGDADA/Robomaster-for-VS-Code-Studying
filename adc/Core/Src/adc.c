/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
#include "adc.h"
#include "stm32f4xx_hal_adc.h"
#include "stm32f4xx_ll_adc.h"
#include <stdint.h>

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;

/* ADC1 init function */
void MX_ADC1_Init(void)
{

    /* USER CODE BEGIN ADC1_Init 0 */

    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */

    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC1_Init 2 */

    /* USER CODE END ADC1_Init 2 */

}
/* ADC3 init function */
void MX_ADC3_Init(void)
{

    /* USER CODE BEGIN ADC3_Init 0 */

    /* USER CODE END ADC3_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC3_Init 1 */

    /* USER CODE END ADC3_Init 1 */

    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc3) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN ADC3_Init 2 */

    /* USER CODE END ADC3_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (adcHandle->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspInit 0 */

        /* USER CODE END ADC1_MspInit 0 */
        /* ADC1 clock enable */
        __HAL_RCC_ADC1_CLK_ENABLE();
        /* USER CODE BEGIN ADC1_MspInit 1 */

        /* USER CODE END ADC1_MspInit 1 */
    }
    else if (adcHandle->Instance == ADC3)
    {
        /* USER CODE BEGIN ADC3_MspInit 0 */

        /* USER CODE END ADC3_MspInit 0 */
        /* ADC3 clock enable */
        __HAL_RCC_ADC3_CLK_ENABLE();

        __HAL_RCC_GPIOF_CLK_ENABLE();
        /**ADC3 GPIO Configuration
        PF10     ------> ADC3_IN8
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        /* USER CODE BEGIN ADC3_MspInit 1 */

        /* USER CODE END ADC3_MspInit 1 */
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

    if (adcHandle->Instance == ADC1)
    {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */

        /* USER CODE END ADC1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_ADC1_CLK_DISABLE();
        /* USER CODE BEGIN ADC1_MspDeInit 1 */

        /* USER CODE END ADC1_MspDeInit 1 */
    }
    else if (adcHandle->Instance == ADC3)
    {
        /* USER CODE BEGIN ADC3_MspDeInit 0 */

        /* USER CODE END ADC3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_ADC3_CLK_DISABLE();

        /**ADC3 GPIO Configuration
        PF10     ------> ADC3_IN8
        */
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_10);

        /* USER CODE BEGIN ADC3_MspDeInit 1 */

        /* USER CODE END ADC3_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
/*
  第一步：校准（开机执行一次）
  用 ADC 读内部固定 1.2V，连续读 200 次求平均
  算出：1个ADC = 多少V（校准系数）
  → 电源 3.3V 飘了，系数自动跟着变，自动修正！
  第二步：测电池电压
  用 ADC 读分压后的电池电压
  采样值 × 校准系数 = 分压后的真实电压
  再 × 分压比（10.09） = 真实电池电压
  第三步：测板载温度
  用 ADC 读内部温度传感器
  采样值 × 校准系数 = 传感器真实电压
  套 ST 官方公式 = 芯片温度
*/
uint16_t get_adcx_value(ADC_HandleTypeDef *hadc,
                        uint32_t channel, uint32_t Offset, uint8_t Rank,
                        uint32_t SamplingTime, uint32_t Timeout)
{
    // 1. 切换ADC通道
    ADC_ChannelConfTypeDef ch_config;
    ch_config.Channel = channel; //内部 1.2V 参考：ADC_CHANNEL_VREFINT
    ch_config.Offset = Offset;  //用来手动修正 ADC 硬件误差的高级功能
    ch_config.Rank = Rank; //这个通道是第几个采
    ch_config.SamplingTime = SamplingTime; //采样时间（ADC 采样多久）
    HAL_ADC_ConfigChannel(&hadc, &ch_config);
    // 2. 启动ADC转换
    HAL_ADC_Start(&hadc);
    // 3. 等待转换完成
    HAL_ADC_PollForConversion(&hadc, Timeout);
    // 4. 读取结果（这就是你问的 HAL_ADC_GetValue）
    uint16_t val = HAL_ADC_GetValue(&hadc);
    return val;
}

float voltage_calibration_coefficient(void)
{
    uint32_t vrefint_adc_sum  = 0;
    for (uint8_t i = 0; i < 200; i++)
    {
        vrefint_adc_sum  += get_adcx_value(&hadc1, ADC_CHANNEL_VREFINT, 0, 1, ADC_SAMPLETIME_480CYCLES, 100);
    }
    float average_vrefint_adc = (float)vrefint_adc_sum  / 200.0f;
    // 把参考电压1.2v（参考电压是固定的而测的电压不固定所以需要基准）分成了average_vrefint_adc个格子 算出每个格子对应值
    // 每格电压 = 内部标准1.2V ÷ 1.2V对应的平均ADC值
    float voltage_per_cell = 1.2f / average_vrefint_adc;
    return voltage_per_cell;
}

float get_battery_voltage(void)
{
    float bat_adc = get_adcx_value(&hadc3, ADC_CHANNEL_8, 0, 1, ADC_SAMPLETIME_480CYCLES, 100);
    // 10.09为分压比
    float bat_voltage = bat_adc * voltage_calibration_coefficient() * 10.09f;
    return bat_voltage;
}

float get_temprate(void)
{
    uint16_t adcx = 0;
    float temperate;
    adcx = get_adcx_value(&hadc1, ADC_CHANNEL_TEMPSENSOR, 0, 1, ADC_SAMPLETIME_480CYCLES, 100);
    temperate = (float)adcx * voltage_calibration_coefficient();
    temperate = (temperate - 0.76f) * 400.0f + 25.0f;
    return temperate;
}
/* USER CODE END 1 */

