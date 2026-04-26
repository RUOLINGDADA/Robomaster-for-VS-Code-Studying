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
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bmi088.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "spi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
BMI088_HandleTypeDef bmi088 = {.hspi = &hspi1,
                               .ACC_CS_GPIO = GPIOA,
                               .ACC_CS_PIN = CS1_Accel_Pin,
                               .GYRO_CS_GPIO = GPIOB,
                               .GYRO_CS_PIN = CS1_Gyro_Pin
                              };

BMI088_RawDataTypeDef raw;
BMI088_PhysDataTypeDef phys;
volatile uint8_t bmi088_drdy_flag = 0;
volatile uint32_t bmi088_irq_count = 0;
volatile uint32_t bmi088_read_count = 0;
volatile uint32_t bmi088_read_error_count = 0;
static uint32_t temp_div = 0;
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
void usart_printf(const char *fmt, ...)
{
    static uint8_t tx_buf[256];  // 单任务下可保留静态，减少栈占用
    uint8_t len;
    va_list ap;

    // 等待上一次UART传输完成
    while (huart1.gState != HAL_UART_STATE_READY)
    {
    }

    // 安全格式化字符串
    va_start(ap, fmt);
    len = vsnprintf((char*)tx_buf, sizeof(tx_buf), fmt, ap);
    va_end(ap);

    // 严谨处理返回值
    if (len <= 0)
    {
        return;  // 格式化出错，直接返回
    }
    if (len >= sizeof(tx_buf))
    {
        len = sizeof(tx_buf) - 1;  // 截断保护
    }

    HAL_UART_Transmit(&huart1, tx_buf, len, 9999);
}

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

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    /* USER CODE BEGIN 2 */
    BMI088_Init(&bmi088,
                BMI088_ACC_RANGE_24G,
                BMI088_ACC_ODR_1600HZ,
                BMI088_GYRO_RANGE_2000DPS,
                BMI088_GYRO_BW_532HZ_2000HZ);
    // BMI088_Init(&bmi088,
    //             BMI088_ACC_RANGE_24G,
    //             BMI088_ACC_ODR_400HZ,
    //             BMI088_GYRO_RANGE_2000DPS,
    //             BMI088_GYRO_BW_47HZ_400HZ);
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // if (bmi088_new_data_exti_flags & 0x01)
        // {
        //     // 读取寄存器清除标志位
        //     if (BMI088_ClearFlag(&bmi088, BMI088_ACC_INT_FLAG) != BMI088_OK)
        //     {
        //         return BMI088_ERROR;
        //     }
        //     BMI088_ReadRaw(&bmi088, &raw);
        //     BMI088_ConvertRawToPhys(&bmi088, &raw, &phys);
        //     bmi088_new_data_exti_flags &= ~BMI088_ACC_INT_FLAG;
        // }
        // else if (bmi088_new_data_exti_flags & 0x02)
        // {
        //     // 读取寄存器清除标志位
        //     if (BMI088_ClearFlag(&bmi088, BMI088_GYRO_INT_FLAG) != BMI088_OK)
        //     {
        //         return BMI088_ERROR;
        //     }
        //     BMI088_ReadRaw(&bmi088, &raw);
        //     BMI088_ConvertRawToPhys(&bmi088, &raw, &phys);
        //     bmi088_new_data_exti_flags &= ~BMI088_GYRO_INT_FLAG;
        // }

        uint8_t flag = 0;

        __disable_irq();
        flag = bmi088_drdy_flag;
        bmi088_drdy_flag = 0;
        __enable_irq();

        if (flag)
        {
            if (BMI088_ReadRaw6Axis(&bmi088, &raw) == BMI088_OK)
            {
                if (++temp_div >= 40)   // 400Hz 下约 10Hz 读温度
                {
                    temp_div = 0;
                    BMI088_ReadTemperature(&bmi088, &raw);
                }

                BMI088_ConvertRawToPhys(&bmi088, &raw, &phys);
                bmi088_read_count++;
            }
            else
            {
                bmi088_read_error_count++;
            }

        }

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
/**
GYRO DRDY INT3 作为唯一采样中断
    ↓
EXTI 回调只置位 flag，不读 SPI
    ↓
main while 中看到 flag
    ↓
一次性 BMI088_ReadRaw() 读取 ACC + GYRO
    ↓
BMI088_ConvertRawToPhys()
    ↓
温度每 100 次或 200 次再读一次
*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // if (GPIO_Pin == INT1_Accel_Pin)
    // {
    //     // 0000 0001
    //     bmi088_new_data_exti_flags |= BMI088_ACC_INT_FLAG;
    // }
    // else if (GPIO_Pin == INT1_Gyro_Pin)
    // {
    //     // 0000 0010
    //     bmi088_new_data_exti_flags |= BMI088_GYRO_INT_FLAG;
    // }
    if (GPIO_Pin == INT1_Gyro_Pin)
    {
        bmi088_drdy_flag = 1;
        bmi088_irq_count++;
    }
}
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
    while (1)
    {
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
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
