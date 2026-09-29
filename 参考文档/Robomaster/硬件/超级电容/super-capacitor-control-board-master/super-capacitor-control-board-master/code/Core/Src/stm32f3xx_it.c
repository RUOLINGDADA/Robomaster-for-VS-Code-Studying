/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32f3xx_it.c
 * @brief   Interrupt Service Routines.
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f3xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "POWER.h"
#include "massage.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PERIOD 10240
#define abs(a) ((a) >= 0) ? (a) : (-a)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern uint8_t PowerFlagSwitch;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan;
extern HRTIM_HandleTypeDef hhrtim1;
extern TIM_HandleTypeDef htim3;
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F3xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f3xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles CAN RX0 interrupt.
  */
void CAN_RX0_IRQHandler(void)
{
  /* USER CODE BEGIN CAN_RX0_IRQn 0 */
  static uint8_t data[8];
  static uint32_t CAN_RX0_count = 0;
  CAN_RxHeaderTypeDef RxMessage;
  /* USER CODE END CAN_RX0_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN CAN_RX0_IRQn 1 */
  CAN_RX0_count++;
  HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &RxMessage, data);
  Can_Receive_Massage(RxMessage,data);
  CAN_ID_Enlist(&can_id_list, RxMessage.StdId);
  /* USER CODE END CAN_RX0_IRQn 1 */
}

/**
  * @brief This function handles CAN RX1 interrupt.
  */
void CAN_RX1_IRQHandler(void)
{
  /* USER CODE BEGIN CAN_RX1_IRQn 0 */
  static uint8_t data[8];
  static uint32_t CAN_RX1_count = 0;
  CAN_RxHeaderTypeDef RxMessage;
  /* USER CODE END CAN_RX1_IRQn 0 */
  HAL_CAN_IRQHandler(&hcan);
  /* USER CODE BEGIN CAN_RX1_IRQn 1 */
  CAN_RX1_count++;
  HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO1, &RxMessage, data);
  CAN_ID_Enlist(&can_id_list, RxMessage.StdId);
  /* USER CODE END CAN_RX1_IRQn 1 */
}

/**
  * @brief This function handles TIM3 global interrupt.
  */
void TIM3_IRQHandler(void)
{
  /* USER CODE BEGIN TIM3_IRQn 0 */
  static uint8_t key1_status = 0, key2_status = 0;
  static uint32_t led_taggle_timestamp;
  /* USER CODE END TIM3_IRQn 0 */
  HAL_TIM_IRQHandler(&htim3);
  /* USER CODE BEGIN TIM3_IRQn 1 */
  //  Resolve_ADC();

#if USE_LED_FISHING
    if (HAL_GetTick() - led_taggle_timestamp >= 500)
    {
      led_taggle_timestamp = HAL_GetTick();
      LED_R_Taggle();
    }
#endif
  Status_Control();
  Power_Error_Taip();
  Power_Limit_Loop();
  Power_Loop_Mode();
  CAN_Send_Message();
  // 读取按钮
  if (Read_Key2() == 0)
  {
    key2_status <= 254 ? key2_status++ : 0;
  }
  if (Read_Key1() == 0)
  {
    key1_status <= 254 ? key1_status++ : 0;
  }
  if (Read_Key_All() == 1 && key2_status > 50 && key1_status > 50)
  {
    if(Power.status.flag.CupEnable == ENABLE)
    {
      Power.status.flag.CupEnable = Power.status.flag.CupEnable = DISABLE ;
      HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
      //清除错误标志位
      Power.error_status.value = 0;
      Power.status.flag.PowerLoopError = DISABLE;
      Power.running_status = Wait;
      Power.power_mode = Standby;
    }
    else
    {
      Power.status.flag.CupEnable = ENABLE;
      Power.error_status.value = 0;
      Power.status.flag.PowerLoopError = DISABLE;
      Power.running_status = Wait;
    }
    key1_status = 0;
    key2_status = 0;
  }
  else if (Read_Key_All() == 1 && key2_status >= 20 && key1_status >= 20)
  {
    Power.status.flag.CupEnable = DISABLE;
    HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);

    key1_status = 0;
    key2_status = 0;
  }
  else if (Read_Key1() == 1 && key1_status > 10)
  {
    menu_pages++;
    menu_refresh = true;
    key1_status = 0;
    key2_status = 0;
  }
  else if (Read_Key2() == 1 && key2_status > 10)
  {
    menu_pages--;
    menu_refresh = true;

    key1_status = 0;
    key2_status = 0;
  }
  // 无任何触发重置
  else if (Read_Key1() == 1 && Read_Key2() == 1)
  {
    key1_status = 0;
    key2_status = 0;
  }

  /* USER CODE END TIM3_IRQn 1 */
}

/**
  * @brief This function handles HRTIM timer A global interrupt.
  */
void HRTIM1_TIMA_IRQHandler(void)
{
  /* USER CODE BEGIN HRTIM1_TIMA_IRQn 0 */
  static int HRTIM_count = 0;
  /* USER CODE END HRTIM1_TIMA_IRQn 0 */
  HAL_HRTIM_IRQHandler(&hhrtim1,HRTIM_TIMERINDEX_TIMER_A);
  /* USER CODE BEGIN HRTIM1_TIMA_IRQn 1 */
  if (HRTIM_count < 1)
  {
    HRTIM_count++;
    return;
  }
  HRTIM_count = 0;
  Resolve_ADC();
  Power_Loop();
  /* USER CODE END HRTIM1_TIMA_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
