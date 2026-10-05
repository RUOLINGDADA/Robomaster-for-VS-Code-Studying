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
#include "c610_m2006.h"
#include "can.h"
#include <string.h>
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

volatile FreeRtosFaultContextTypeDef g_freertos_fault_context;

static void FreeRtos_RecordFault(uint32_t signature, TaskHandle_t task) {
  g_freertos_fault_context.signature = signature;
  g_freertos_fault_context.task_handle = (uint32_t)(uintptr_t)task;
  g_freertos_fault_context.stack_high_water_words = task != NULL
      ? (uint32_t)uxTaskGetStackHighWaterMark(task) : 0U;
  g_freertos_fault_context.free_heap_bytes = xPortGetFreeHeapSize();
  g_freertos_fault_context.scheduler_state =
      (uint32_t)xTaskGetSchedulerState();
  memset((void *)g_freertos_fault_context.task_name, 0,
         sizeof(g_freertos_fault_context.task_name));
  if (task != NULL) {
    const char *name = pcTaskGetName(task);
    if (name != NULL) {
      strncpy((char *)g_freertos_fault_context.task_name, name,
              sizeof(g_freertos_fault_context.task_name) - 1U);
    }
  }
}

/* configCHECK_FOR_STACK_OVERFLOW=2：FreeRTOS 在每次切换任务时调用。 */
void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name) {
  (void)task_name;
  FreeRtos_RecordFault(0x53544B4FU, task); /* STKO */
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}

/* configUSE_MALLOC_FAILED_HOOK=1：保留堆耗尽现场，避免继续运行破坏内核链表。 */
void vApplicationMallocFailedHook(void) {
  FreeRtos_RecordFault(0x4D414C4CU, xTaskGetCurrentTaskHandle()); /* MALL */
  taskDISABLE_INTERRUPTS();
  for (;;) {
  }
}

/* USER CODE END Variables */
/* Definitions for task_feed_motor */
osThreadId_t task_feed_motorHandle;
const osThreadAttr_t task_feed_motor_attributes = {
  .name = "task_feed_motor",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for task_yaw */
osThreadId_t task_yawHandle;
const osThreadAttr_t task_yaw_attributes = {
  .name = "task_yaw",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for task_pitch */
osThreadId_t task_pitchHandle;
const osThreadAttr_t task_pitch_attributes = {
  .name = "task_pitch",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for task_dbus */
osThreadId_t task_dbusHandle;
const osThreadAttr_t task_dbus_attributes = {
  .name = "task_dbus",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void task_feed_motor_entry(void *argument);
void task_yaw_entry(void *argument);
void task_pitch_entry(void *argument);
void task_dbus_entry(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

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

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of task_feed_motor */
  task_feed_motorHandle = osThreadNew(task_feed_motor_entry, NULL, &task_feed_motor_attributes);

  /* creation of task_yaw */
  task_yawHandle = osThreadNew(task_yaw_entry, NULL, &task_yaw_attributes);

  /* creation of task_pitch */
  task_pitchHandle = osThreadNew(task_pitch_entry, NULL, &task_pitch_attributes);

  /* creation of task_dbus */
  task_dbusHandle = osThreadNew(task_dbus_entry, NULL, &task_dbus_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_task_feed_motor_entry */
/**
  * @brief  Function implementing the task_feed_motor thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_task_feed_motor_entry */
__weak void task_feed_motor_entry(void *argument)
{
  /* USER CODE BEGIN task_feed_motor_entry */
  (void)argument;
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END task_feed_motor_entry */
}

/* USER CODE BEGIN Header_task_yaw_entry */
/**
  * @brief  云台底盘（Yaw/水平轴）任务的 CubeMX 默认弱实现（没有强实现时只让出 CPU）。
  * @param  argument 任务参数，当前未使用（保留 FreeRTOS 入口签名）。
  * @retval None 任务不会返回（循环通过 osDelay 让出调度）。
  *
  * @note   这里先保留安全空任务，避免用户尚未完成 Yaw 句柄初始化时
  *         意外给 GM6020 输出电流。后续应在独立任务源文件中提供同名强
  *         定义，并在任务循环内调用 Gm6020_Process()/Gm6020_Send()。
  */
/* USER CODE END Header_task_yaw_entry */
__weak void task_yaw_entry(void *argument)
{
  /* USER CODE BEGIN task_yaw_entry */
  (void)argument;
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END task_yaw_entry */
}

/* USER CODE BEGIN Header_task_pitch_entry */
/**
  * @brief  发射云台（Pitch/纵向轴）任务的 CubeMX 默认弱实现（没有强实现时保持安全空转）。
  * @param  argument 任务参数，当前未使用（保留 FreeRTOS 入口签名）。
  * @retval None 任务不会返回（循环通过 osDelay 让出调度）。
  *
  * @note   Pitch 通常使用 GM6020 反馈 ID 0x206。正式实现时应在本任务
  *         中使用独立的 Gm6020_HandleTypeDef，并保持输出默认关闭；不能
  *         复用 Yaw 任务的句柄，否则两个轴的目标电流和限位状态会互相覆盖。
  */
/* USER CODE END Header_task_pitch_entry */
__weak void task_pitch_entry(void *argument)
{
  /* USER CODE BEGIN task_pitch_entry */
  (void)argument;
  /* Infinite loop */
  for(;;)
  {
    /* 空任务只让出 CPU，不初始化电机、不发送电流（避免弱实现误驱动硬件）。 */
    osDelay(1);
  }
  /* USER CODE END task_pitch_entry */
}

/* USER CODE BEGIN Header_task_dbus_entry */
/**
 * @brief DBUS 任务安全弱实现（未链接强入口时不启动 DMA、不发布命令）。
 * @param argument 未使用的任务参数。
 * @retval None 循环让出 CPU，不会返回。
 */
/* USER CODE END Header_task_dbus_entry */
__weak void task_dbus_entry(void *argument)
{
  /* USER CODE BEGIN task_dbus_entry */
  (void)argument;
  for (;;) {
    osDelay(1);
  }
  /* USER CODE END task_dbus_entry */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

