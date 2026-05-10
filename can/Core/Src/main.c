/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "can.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can_ex.h"
#include <stdint.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
CAN_HandleTypeDefEx *hcan1_ex;
uint8_t rx_data[8];
#define USART_DEBUG
#ifdef USART_DEBUG
uint8_t debug_print_flag = 0;
uint32_t rx_std_id; // 标准ID(11位)
uint32_t rx_ext_id; // 扩展ID(29位)
uint8_t rx_ide;     // IDE位(0=标准帧, 1=扩展帧)
uint8_t rx_rtr;     // RTR位(0=数据帧, 1=远程帧)
uint8_t rx_dlc;     // 数据长度码(0-8)
#endif
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  hcan1_ex = CAN_Create(&hcan1, CAN_PLATFORM_STM32F4);
  if (hcan1_ex == NULL) {
    Error_Handler();
  }
  // 使能接收FIFO0消息挂起中断
  CAN_EnableInterrupt(hcan1_ex, CAN_IT_RX_FIFO0_MSG_PENDING);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t _random = 0;
    _random = random();
    uint8_t tx_data[8] = {0x01 + _random, 0x02 + _random, 0x03 + _random,
                          0x04 + _random, 0x05 + _random, 0x06 + _random,
                          0x07 + _random, 0x08 + _random};
    uint32_t tx_mailbox; // 存储哪个邮箱发送的

    tx_header.StdId = 0x123;
    tx_header.ExtId = 0x00;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = 8;
    tx_header.TransmitGlobalTime = DISABLE;

    CAN_SendData(hcan1_ex, &tx_header, tx_data, &tx_mailbox);
    HAL_Delay(1000);
#ifdef USART_DEBUG
    if (debug_print_flag == 1) {
      usart_printf("[%5lu] ", HAL_GetTick());

      // 打印帧类型和ID类型
      if (rx_rtr == CAN_RTR_REMOTE) {
        usart_printf("REMOTE ");
      } else {
        usart_printf("DATA   ");
      }

      if (rx_ide == CAN_ID_STD) {
        usart_printf("STD ID: 0x%03X | DLC: %d | Data: ", rx_std_id, rx_dlc);
      } else {
        usart_printf("EXT ID: 0x%08X | DLC: %d | Data: ", rx_ext_id, rx_dlc);
      }

      // 打印数据段(远程帧无数据)
      if (rx_rtr == CAN_RTR_DATA) {
        for (uint8_t i = 0; i < rx_dlc; i++) {
          usart_printf("0x%02X ", rx_data[i]);
        }
      } else {
        usart_printf("(No Data)");
      }

      usart_printf("\r\n");
      debug_print_flag = 0;
    }
#endif
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// 接收完成回调函数
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header;
  // 接收数据
  if (CAN_ReceiveData(hcan1_ex, CAN_RX_FIFO0, &rx_header, rx_data) == CAN_OK) {
#ifdef USART_DEBUG
    debug_print_flag = 1;
    rx_std_id = rx_header.StdId;
    rx_ext_id = rx_header.ExtId;
    rx_ide = rx_header.IDE;
    rx_rtr = rx_header.RTR;
    rx_dlc = rx_header.DLC;
#endif
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
