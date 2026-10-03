/* USER CODE BEGIN Header */
/*
 * FreeRTOS Kernel V10.3.1
 * Portion Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Portion Copyright (C) 2019 StMicroelectronics, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */
/* USER CODE END Header */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H /* 防止 FreeRTOS 工程配置被重复包含。 */

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section of the
 * FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* USER CODE BEGIN Includes */
/* Section where include file can be added */
/* USER CODE END Includes */

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32f4xx.h" /* CMSIS-RTOS 适配层使用的 MCU 头文件。 */
#endif /* CMSIS_device_header */

#define configENABLE_FPU                         0 /* 不启用 FreeRTOS FPU 上下文扩展。 */
#define configENABLE_MPU                         0 /* 不启用 Cortex-M4 MPU 扩展。 */

#define configUSE_PREEMPTION                     1 /* 允许高优先级任务抢占低优先级任务。 */
#define configSUPPORT_STATIC_ALLOCATION          1 /* 允许静态创建 RTOS 对象。 */
#define configSUPPORT_DYNAMIC_ALLOCATION         1 /* 允许 CMSIS 动态创建任务。 */
#define configUSE_IDLE_HOOK                      0 /* 不注册 Idle Hook。 */
#define configUSE_TICK_HOOK                      0 /* 不注册系统 Tick Hook。 */
#define configCPU_CLOCK_HZ                       ( SystemCoreClock ) /* CPU 时钟来源。 */
#define configTICK_RATE_HZ                       ((TickType_t)1000) /* RTOS Tick 频率，Hz。 */
#define configMAX_PRIORITIES                     ( 56 ) /* 最大任务优先级数量。 */
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128) /* Idle 任务最小栈深度。 */
#define configTOTAL_HEAP_SIZE                    ((size_t)15360) /* FreeRTOS heap_4 堆大小，字节。 */
#define configMAX_TASK_NAME_LEN                  ( 16 ) /* 任务名最大字符数。 */
#define configUSE_TRACE_FACILITY                 1 /* 保留任务跟踪所需数据。 */
#define configUSE_16_BIT_TICKS                   0 /* 使用 32 位 Tick，延长回绕周期。 */
#define configUSE_MUTEXES                        1 /* 启用互斥锁。 */
#define configQUEUE_REGISTRY_SIZE                8 /* 可注册到调试器的队列/信号量数量。 */
#define configUSE_RECURSIVE_MUTEXES              1 /* 启用递归互斥锁。 */
#define configUSE_COUNTING_SEMAPHORES            1 /* 启用计数信号量。 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0 /* 使用通用任务选择路径。 */
/* USER CODE BEGIN MESSAGE_BUFFER_LENGTH_TYPE */
/* Defaults to size_t for backward compatibility, but can be changed
   if lengths will always be less than the number of bytes in a size_t. */
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t /* 消息缓冲区长度字段类型。 */
/* USER CODE END MESSAGE_BUFFER_LENGTH_TYPE */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0 /* 不使用旧式协程。 */
#define configMAX_CO_ROUTINE_PRIORITIES          ( 2 ) /* 协程功能关闭时的默认占位值。 */

/* Software timer definitions. */
#define configUSE_TIMERS                         1 /* 启用软件定时器。 */
#define configTIMER_TASK_PRIORITY                ( 2 ) /* 软件定时器服务任务优先级。 */
#define configTIMER_QUEUE_LENGTH                 10 /* 定时器命令队列长度。 */
#define configTIMER_TASK_STACK_DEPTH             256 /* 定时器服务任务栈深度。 */

/* CMSIS-RTOS V2 flags */
#define configUSE_OS2_THREAD_SUSPEND_RESUME  1 /* 启用 CMSIS-RTOS 线程挂起/恢复。 */
#define configUSE_OS2_THREAD_ENUMERATE       1 /* 启用 CMSIS-RTOS 线程枚举。 */
#define configUSE_OS2_EVENTFLAGS_FROM_ISR    1 /* 允许 ISR 设置 CMSIS 事件标志。 */
#define configUSE_OS2_THREAD_FLAGS           1 /* 启用 CMSIS 线程标志。 */
#define configUSE_OS2_TIMER                  1 /* 启用 CMSIS 软件定时器。 */
#define configUSE_OS2_MUTEX                  1 /* 启用 CMSIS 互斥量。 */

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet             1 /* 编译任务优先级设置 API。 */
#define INCLUDE_uxTaskPriorityGet            1 /* 编译任务优先级查询 API。 */
#define INCLUDE_vTaskDelete                  1 /* 编译删除任务 API。 */
#define INCLUDE_vTaskCleanUpResources        0 /* 不编译旧资源清理 API。 */
#define INCLUDE_vTaskSuspend                 1 /* 编译任务挂起 API。 */
#define INCLUDE_vTaskDelayUntil              1 /* 编译固定周期调度 API。 */
#define INCLUDE_vTaskDelay                   1 /* 编译相对延时 API。 */
#define INCLUDE_xTaskGetSchedulerState       1 /* 编译调度器状态查询 API。 */
#define INCLUDE_xTimerPendFunctionCall       1 /* 编译定时器服务任务回调 API。 */
#define INCLUDE_xQueueGetMutexHolder         1 /* 编译互斥锁持有者查询 API。 */
#define INCLUDE_uxTaskGetStackHighWaterMark  1 /* 编译任务栈余量查询 API。 */
#define INCLUDE_xTaskGetCurrentTaskHandle    1 /* 编译当前任务句柄查询 API。 */
#define INCLUDE_eTaskGetState                1 /* 编译任务状态查询 API。 */

/*
 * The CMSIS-RTOS V2 FreeRTOS wrapper is dependent on the heap implementation used
 * by the application thus the correct define need to be enabled below
 */
#define USE_FreeRTOS_HEAP_4 /* 选择 FreeRTOS heap_4 内存管理实现。 */

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
 /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 #define configPRIO_BITS         __NVIC_PRIO_BITS /* 从 CMSIS 读取 NVIC 优先级位数。 */
#else
 #define configPRIO_BITS         4 /* STM32F407 的 NVIC 优先级位数。 */
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15 /* FreeRTOS 可用的最低中断优先级。 */

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5 /* 可调用 FromISR API 的最高优先级边界。 */

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY 		( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) ) /* 内核中断优先级寄存器值。 */
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) ) /* 可调用 RTOS ISR API 的硬件优先级值。 */

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
/* USER CODE BEGIN 1 */
#define configASSERT( x ) if ((x) == 0) {taskDISABLE_INTERRUPTS(); for( ;; );} /* 断言失败后关闭中断并停机。 */
/* USER CODE END 1 */

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler    SVC_Handler /* FreeRTOS SVC 入口映射到 STM32 启动文件。 */
#define xPortPendSVHandler PendSV_Handler /* FreeRTOS PendSV 入口映射到 STM32 启动文件。 */

/* IMPORTANT: After 10.3.1 update, Systick_Handler comes from NVIC (if SYS timebase = systick), otherwise from cmsis_os2.c */

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0 /* 使用 FreeRTOS 默认 SysTick 实现。 */

/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */
/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
