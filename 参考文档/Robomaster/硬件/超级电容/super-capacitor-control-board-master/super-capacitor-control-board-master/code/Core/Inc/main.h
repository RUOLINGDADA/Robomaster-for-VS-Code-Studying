/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
#define USE_FULL_ASSERT                 // 是否使用断言函数

#define DEBUG_ADC_CALIBRATION         0   // ADC采样标定

#if !(DEBUG_ADC_CALIBRATION)
#define USE_LED_FISHING               1   // 是否使用LED闪烁显示工作状态
#define USE_POWER_LIMIT               1   // 是否使用功率限制
#define USE_POWER_LOOP                1   // 启用功率环路
#endif

#define USE_CUP_UNDERVOLTAGE_CLOSE    1   // 电容欠压关断
#define USE_CUP_FULL_CLOSE            0   // 电容满电关断
#define USE_ERROR_AUTO_RESTORE        1   // 启用错误自动恢复
#define USE_POWER_BUFFER_LOOP         1   // 启用功率缓冲环

#define ADC_CALIBRATION_DATA_SIZE     10  // 标定数据大小

#define ADC_CALIBRATION_VOL_B         3   // 电压标定值偏移量
#define ADC_CALIBRATION_VOL_K         3   // 电压标定值增量
#define ADC_CALIBRATION_CUR_B         3   // 电流标定值偏移量
#define ADC_CALIBRATION_CUR_K         3   // 电流标定值增量

#define DELETE_VIN_SAMPLE             1   // 删除输入采样

/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include "arm_math.h"
#include "tim.h"
#include "mathfun.h"
#include "PID.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

typedef uint32_t Timestamp_t;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

extern uint8_t menu_pages;
extern bool menu_refresh;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_2_Pin GPIO_PIN_14
#define KEY_2_GPIO_Port GPIOC
#define KEY_1_Pin GPIO_PIN_15
#define KEY_1_GPIO_Port GPIOC
#define ADC_Vout_Pin GPIO_PIN_0
#define ADC_Vout_GPIO_Port GPIOA
#define ADC_Vin_Pin GPIO_PIN_1
#define ADC_Vin_GPIO_Port GPIOA
#define ADC_Vcup_Pin GPIO_PIN_2
#define ADC_Vcup_GPIO_Port GPIOA
#define ADC_Icup_Pin GPIO_PIN_3
#define ADC_Icup_GPIO_Port GPIOA
#define ADC_Iin_Pin GPIO_PIN_4
#define ADC_Iin_GPIO_Port GPIOA
#define ADC_Iout_Pin GPIO_PIN_6
#define ADC_Iout_GPIO_Port GPIOA
#define LED_R_Pin GPIO_PIN_12
#define LED_R_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_13
#define LED_G_GPIO_Port GPIOB
#define LED_B_Pin GPIO_PIN_14
#define LED_B_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_12
#define BUZZER_GPIO_Port GPIOA
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_7
#define I2C_SDA_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define PERIOD 11520
// LED控制
#define LED_R(x) HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, x)
#define LED_G(x) HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, x)
#define LED_B(x) HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, x)
#define LED_R_Taggle() HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin)
#define LED_G_Taggle() HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin)
#define LED_B_Taggle() HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin)
// 蜂鸣器控制
#define BUZZER(x) __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, (x) ? 5 : 0);
// 按钮读取 已做硬件消抖
#define Read_Key1() HAL_GPIO_ReadPin(KEY_1_GPIO_Port, KEY_1_Pin)
#define Read_Key2() HAL_GPIO_ReadPin(KEY_2_GPIO_Port, KEY_2_Pin)
#define Read_Key_All() (Read_Key1() || Read_Key2())
// HRTIM占空比设置 靠近电容侧是B，此处占空比位下管占空比。采样位置设置以电流采样位置为主
// HRTIMA：计数到cmp1时输出0,HRTIMB：计数到cmp1时输出1 
#define Set_HRTIMA(x) (HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP1xR = (PERIOD) - (x))
#define Set_HRTIMB(x) (HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_B].CMP1xR = (x))
#define Set_Sample(x) (HRTIM1->sTimerxRegs[HRTIM_TIMERINDEX_TIMER_A].CMP3xR = (PERIOD) - (x))

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
