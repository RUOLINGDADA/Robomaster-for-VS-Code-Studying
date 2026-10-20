/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

#include <stdint.h>
#define FREERTOS_FAULT_TASK_NAME_LENGTH 16U
typedef struct {
  volatile uint32_t signature;
  volatile uint32_t task_handle;
  volatile uint32_t stack_high_water_words;
  volatile uint32_t free_heap_bytes;
  volatile uint32_t scheduler_state;
  char task_name[FREERTOS_FAULT_TASK_NAME_LENGTH];
} FreeRtosFaultContextTypeDef;
typedef struct {
  volatile uint32_t signature;
  volatile uint32_t ipsr;
  volatile uint32_t control;
  volatile uint32_t msp;
  volatile uint32_t psp;
  volatile uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
  volatile uint32_t cfsr, hfsr, dfsr, afsr, bfar, mmfar;
} HardFaultContextTypeDef;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

extern volatile FreeRtosFaultContextTypeDef g_freertos_fault_context;
extern volatile HardFaultContextTypeDef g_hardfault_context;

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI_MISO_Pin GPIO_PIN_4
#define SPI_MISO_GPIO_Port GPIOB
#define SPI1_CLK_Pin GPIO_PIN_3
#define SPI1_CLK_GPIO_Port GPIOB
#define RSTN_IST8310_Pin GPIO_PIN_6
#define RSTN_IST8310_GPIO_Port GPIOG
#define DRDY_IST8310_Pin GPIO_PIN_3
#define DRDY_IST8310_GPIO_Port GPIOG
#define DRDY_IST8310_EXTI_IRQn EXTI3_IRQn
#define CS1_Accel_Pin GPIO_PIN_4
#define CS1_Accel_GPIO_Port GPIOA
#define INT1_Accel_Pin GPIO_PIN_4
#define INT1_Accel_GPIO_Port GPIOC
#define INT1_Accel_EXTI_IRQn EXTI4_IRQn
#define INT1_Gyro_Pin GPIO_PIN_5
#define INT1_Gyro_GPIO_Port GPIOC
#define INT1_Gyro_EXTI_IRQn EXTI9_5_IRQn
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define CS1_Gyro_Pin GPIO_PIN_0
#define CS1_Gyro_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
