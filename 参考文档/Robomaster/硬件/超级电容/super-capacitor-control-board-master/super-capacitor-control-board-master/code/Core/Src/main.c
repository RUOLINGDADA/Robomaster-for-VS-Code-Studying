/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "hrtim.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "POWER.h"
#include "sample.h"
#include "massage.h"
#include "cap.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 显示值
static float Vc, Vi, Vo, Ii, Io, Ic, Pc, Pi, Po, Ps, Wb, Pch, Pco, Emax, Enow,C;
// 菜单页
uint8_t menu_pages = 0;
bool menu_refresh;
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
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  Init_ADC_Data();
  Init_Power();
  Init_Power_Limit();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_HRTIM1_Init();
  MX_ADC2_Init();
  MX_TIM3_Init();
  MX_CAN_Init();
  MX_USART2_UART_Init();
  MX_TIM16_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  BUZZER(1);
  OLED_Init();
  OLED_ShowString(1, 2, "Loading...");
  Init_ADC_Config();
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  Set_HRTIMA(0);
  Set_HRTIMB(0);
  HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_A); // 开启PWM输出和PWM计时器
  HAL_HRTIM_WaveformCountStart(&hhrtim1, HRTIM_TIMERID_TIMER_B); // 开启PWM输出和PWM计时器
  HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);//启动时关闭

  CAN_Filter_Config();
  HAL_Delay(500);
  HAL_TIM_Base_Start_IT(&htim3);                                                     // 启动定时器3 200Hz
  __HAL_HRTIM_TIMER_ENABLE_IT(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_TIM_IT_REP); // 使能HRTIM
  BUZZER(0);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    HAL_IWDG_Refresh(&hiwdg); // 喂狗
    Vc = Vcap.ave_value;
    Vi = Vin.ave_value;
    Vo = Vout.ave_value;
    Ii = Iin.ave_value;
    Io = Iout.ave_value;
    Ic = Icap.ave_value;
    Po = Power_limit.data.Pout;
    Pi = Power_limit.data.Pin;
    Ps = Power_limit.data.Pset;
    Wb = Power_limit.data.power_buffer;
    Pch = Power_limit.config_data.Icharge_max * Vout.ave_value;
    Pco = Power_limit.config_data.Icompensate_max * Vout.ave_value;
    //E=Q*U
    Emax = Cap_data.energy_data.cap_energy_max;
    Enow = Cap_data.energy_data.cap_energy_value;
    C=Cap_data.parameter_data.cap_value;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    Can_ID_Delist(&can_id_list);
    Can_ID_Sort(&can_id_list);
    if (menu_refresh)
    {
      menu_refresh = false;
      OLED_Clear();
    }
    switch (menu_pages)
    {
    /* 电源环路数据显示 */
    case 0:
      OLED_ShowString(2, 1, "Vi:");
      OLED_ShowNum(2, 4, Floor(Vi), 2);
      OLED_ShowChar(2, 6, '.');
      OLED_ShowNum(2, 7, Mod1(Vi) / 10, 1);

      OLED_ShowString(2, 9, "Ii:");
      OLED_ShowChar(2, 12, Ii >= 0 ? ' ' : '-');
      OLED_ShowNum(2, 13, Floor(abs(Ii)), 1);
      OLED_ShowChar(2, 14, '.');
      OLED_ShowNum(2, 15, Mod1(abs(Ii)), 2);

      OLED_ShowString(3, 1, "Vo:");
      OLED_ShowNum(3, 4, Floor(Vo), 2);
      OLED_ShowChar(3, 6, '.');
      OLED_ShowNum(3, 7, Mod1(Vo) / 10, 1);

      OLED_ShowString(3, 9, "Io:");
      OLED_ShowChar(3, 12, Io >= 0 ? ' ' : '-');
      OLED_ShowNum(3, 13, Floor(abs(Io)), 1);
      OLED_ShowChar(3, 14, '.');
      OLED_ShowNum(3, 15, Mod1(abs(Io)), 2);

      OLED_ShowString(4, 1, "Vc:");
      OLED_ShowNum(4, 4, Floor(Vc), 2);
      OLED_ShowChar(4, 6, '.');
      OLED_ShowNum(4, 7, Mod1(Vc) / 10, 1);

      OLED_ShowString(4, 9, "Ic:");
      OLED_ShowChar(4, 12, Ic >= 0 ? ' ' : '-');
      OLED_ShowNum(4, 13, Floor(abs(Ic)), 1);
      OLED_ShowChar(4, 14, '.');
      OLED_ShowNum(4, 15, Mod1(abs(Ic)), 2);

      switch (Power.power_mode)
      {
      case Standby:
        OLED_ShowString(1, 1, "Standby   ");
        break;
      case Boost:
        OLED_ShowString(1, 1, "Boost     ");
        break;
      case Buck:
        OLED_ShowString(1, 1, "Buck      ");
        break;
      // case Buck_Boost:
      //   OLED_ShowString(1, 1, "BuckBoost ");
        break;
      }
      switch (Power.running_status)
      {
      case Wait:
        OLED_ShowString(1, 11, "Wait  ");
        break;
      case Run:
        OLED_ShowString(1, 11, "Run   ");
        break;
      case Start:
        OLED_ShowString(1, 11, "Start ");
        break;
      case Error:
        OLED_ShowString(1, 11, "Error ");
        break;
      case ModeSwitch:
        OLED_ShowString(1, 11, "Switch");
        break;
      }
      break;
    /* 功率环路数据显示 */
    case 1:
      OLED_ShowString(1, 1, "Power meter");
      OLED_ShowString(2, 1, "Pi:");
      OLED_ShowChar(2, 4, Pi >= 0 ? ' ' : '-');
      OLED_ShowNum(2, 5, Floor(abs(Pi)), 3);

      OLED_ShowString(2, 9, "Po:");
      OLED_ShowChar(2, 12, Po >= 0 ? ' ' : '-');
      OLED_ShowNum(2, 13, Floor(abs(Po)), 3);

      OLED_ShowString(3, 1, "Ps:");
      OLED_ShowChar(3, 4, Ps >= 0 ? ' ' : '-');
      OLED_ShowNum(3, 5, Floor(abs(Ps)), 3);

      OLED_ShowString(3, 9, "Wb: ");
      OLED_ShowNum(3, 13, Floor(abs(Wb)), 3);

      OLED_ShowString(4, 1, "Pch:");
      OLED_ShowNum(4, 5, Floor(abs(Pch)), 3);

      OLED_ShowString(4, 9, "Pco:");
      OLED_ShowNum(4, 13, Floor(abs(Pco)), 3);

      break;
    /* 状态显示 */
    case 2:
      OLED_ShowString(1, 1, "Status");

      OLED_ShowString(2, 1, "CE:");
      OLED_ShowChar(2, 4, Power.status.flag.CupEnable ? '1' : '0');
      OLED_ShowString(2, 6, "CU:");
      OLED_ShowChar(2, 9, Power.status.flag.CupUndervoltage ? '1' : '0');
      OLED_ShowString(2, 11, "PL:");
      OLED_ShowChar(2, 14, Power.status.flag.PowerCompensationLimit ? '1' : '0');

      OLED_ShowString(3, 1,  "Er:");
      OLED_ShowChar(3, 4, Power.status.flag.PowerLoopError ? '1' : '0');
      OLED_ShowString(3, 6,  "IO:");
      OLED_ShowChar(3, 9, Power.error_status.flag.LoopOvercurrent ? '1' : '0');
      OLED_ShowString(3, 11, "CO:");
      OLED_ShowChar(3, 14, Power.error_status.flag.CupOvervoltage ? '1' : '0');

      OLED_ShowString(4, 1,  "LC:");
      OLED_ShowChar(4, 4, Power.status.flag.CanLossOfConnection ? '1' : '0');
      OLED_ShowString(4, 6,  "LM:");
      OLED_ShowNum(4, 9, Power_limit.status.flag.power_limit_mode,1);
      OLED_ShowString(4, 11, "LB:");
      OLED_ShowChar(4, 14, Power.error_status.flag.InsufficientPowerBuffer ? '1' : '0');
      break;
    /* 电容状态显示 */
    case 3:
      OLED_ShowString(1, 1, "Cap Data");
      OLED_ShowString(2, 1, "Emax:");
      OLED_ShowNum(2, 6, Floor(Emax), 4);
      OLED_ShowString(4,10,"C:");
      OLED_ShowNum(4, 12, Floor(C), 2);
      OLED_ShowChar(4, 14, '.');
      OLED_ShowNum(4, 15, Mod1(C)/10, 2);

      OLED_ShowString(3, 1, "Enow:");
      OLED_ShowNum(3, 6, Floor(Enow), 4);
      OLED_ShowString(4, 1, "Per:");
      OLED_ShowNum(4, 5, Floor(Enow / Emax * 100), 3);
      OLED_ShowChar(4, 8, '%');
      break;
    /* 在线CAN ID显示 */
    case 4:
      OLED_ShowString(1, 1, "CAN ID List1");
      for (uint8_t i = 0; i < 12; i++)
      {
        if (Get_CAN_ID_List_Elem_ID(&can_id_list, i) != 0 && i < Get_CAN_ID_Num(&can_id_list))
        {
          OLED_ShowHexNum((i / 4) % 3 + 2, (i % 4) * 4 + 1, Get_CAN_ID_List_Elem_ID(&can_id_list, i), 3);
        }
        else
        {
          OLED_ShowString((i / 4) % 3 + 2, (i % 4) * 4 + 1, "   ");
        }
      }
      break;
    case 5:
      // 若can ID数大于12进入can ID第二页 否则进入下一页
      if (Get_CAN_ID_Num(&can_id_list) >= 12)
      {
        OLED_ShowString(1, 1, "CAN ID List2");
        for (uint8_t i = 12; i < 24; i++)
        {
          if (Get_CAN_ID_List_Elem_ID(&can_id_list, i) != 0 && i < Get_CAN_ID_Num(&can_id_list))
          {
            OLED_ShowHexNum((i / 4) % 3 + 2, (i % 4) * 4 + 1, Get_CAN_ID_List_Elem_ID(&can_id_list, i), 3);
          }
          else
          {
            OLED_ShowString((i / 4) % 3 + 2, (i % 4) * 4 + 1, "   ");
          }
        }
        break;
      }
      menu_pages++;
      OLED_Clear();
    case 6:
      // 若can ID数大于24进入can ID第二页 否则进入下一页
      if (Get_CAN_ID_Num(&can_id_list) >= 24)
      {
        OLED_ShowString(1, 1, "CAN ID List2");
        for (uint8_t i = 24; i < 32; i++)
        {
          if (Get_CAN_ID_List_Elem_ID(&can_id_list, i) != 0 && i < Get_CAN_ID_Num(&can_id_list))
          {
            OLED_ShowHexNum((i / 4) % 3 + 2, (i % 4) * 4 + 1, Get_CAN_ID_List_Elem_ID(&can_id_list, i), 3);
          }
          else
          {
            OLED_ShowString((i / 4) % 3 + 2, (i % 4) * 4 + 1, "   ");
          }
        }
        if(Get_CAN_ID_Num(&can_id_list)>32)
        {
          OLED_ShowString(4,1, "Too mamy no show");
        }
        break;
      }
      menu_pages++;
      OLED_Clear();
    default:
      menu_pages = 0;
      break;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_HRTIM1;
  PeriphClkInit.Hrtim1ClockSelection = RCC_HRTIM1CLK_PLLCLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  __NVIC_SystemReset();

  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  // 关闭总中断
  __disable_irq();
  // 关闭控制器输出
  HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TA2 | HRTIM_OUTPUT_TB1 | HRTIM_OUTPUT_TB2);
  // 输出错误信息
  OLED_ShowString(1, 1, "Error:AssertFail");
  OLED_ShowString(2, 1, "F:");
  OLED_ShowString(2, 3, file);
  OLED_ShowString(3, 1, "L:");
  OLED_ShowNum(3, 3, line, 6);
  // 打开蜂鸣器
  BUZZER(1);
  // 死循环
  while (1)
  {
    HAL_IWDG_Refresh(&hiwdg); // 喂狗
    BUZZER(1);
    HAL_Delay(200);
    BUZZER(0);
    HAL_Delay(200);
  }
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
