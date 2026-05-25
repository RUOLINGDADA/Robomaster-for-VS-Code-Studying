/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "key.h"
#include "key_task.h"
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for LED_Flash_Task */
osThreadId_t LED_Flash_TaskHandle;
const osThreadAttr_t LED_Flash_Task_attributes = {
  .name = "LED_Flash_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for KeyTask */
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for msgHandleTask */
osThreadId_t msgHandleTaskHandle;
const osThreadAttr_t msgHandleTask_attributes = {
  .name = "msgHandleTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for usartQueue */
osMessageQueueId_t usartQueueHandle;
const osMessageQueueAttr_t usartQueue_attributes = {
  .name = "usartQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Start_LED_Flash_Task(void *argument);
void StartKeyTask(void *argument);
void StartMsgHandleTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  KEY_HandleTypeDef *hkey = Key_Create(KEY_GPIO_Port, KEY_Pin);
  if (hkey == NULL) {
    Error_Handler();
  }
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of usartQueue */
  usartQueueHandle = osMessageQueueNew (16, sizeof(Msg_t*), &usartQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of LED_Flash_Task */
  LED_Flash_TaskHandle = osThreadNew(Start_LED_Flash_Task, NULL, &LED_Flash_Task_attributes);

  /* creation of KeyTask */
  KeyTaskHandle = osThreadNew(StartKeyTask, (void*) hkey, &KeyTask_attributes);

  /* creation of msgHandleTask */
  msgHandleTaskHandle = osThreadNew(StartMsgHandleTask, NULL, &msgHandleTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Start_LED_Flash_Task */
/**
 * @brief Function implementing the LED_Flash_Task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Start_LED_Flash_Task */
__weak void Start_LED_Flash_Task(void *argument)
{
  /* USER CODE BEGIN Start_LED_Flash_Task */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END Start_LED_Flash_Task */
}

/* USER CODE BEGIN Header_StartKeyTask */
/**
 * @brief Function implementing the KeyTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartKeyTask */
__weak void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  /* Infinite loop */
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartMsgHandleTask */
/**
* @brief Function implementing the msgHandleTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMsgHandleTask */
__weak void StartMsgHandleTask(void *argument)
{
  /* USER CODE BEGIN StartMsgHandleTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartMsgHandleTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

