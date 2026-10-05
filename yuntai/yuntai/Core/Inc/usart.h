/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */

/**
  * @brief  使用 USART1 DMA 输出格式化调试文本（把日志交给 DMA 异步发送）。
  * @param  fmt printf 风格格式字符串（格式化结果写入 DMA 共享缓冲区）。
  * @param  ... 与 fmt 对应的参数（参数只在本次调用内使用）。
  * @note   非阻塞；串口忙时丢弃本条普通日志，不能在 ISR 中调用（避免 ISR 格式化和等待）。
  */
void usart_printf(const char *fmt, ...);

/**
  * @brief  非阻塞尝试发送一条格式化调试文本。
  * @param  fmt printf 格式串；后续参数与格式相同（结果必须放入固定缓冲区）。
  * @retval true 已接受到 DMA；false 串口忙、在 ISR 调用或文本超过缓冲区（调用者可重试）。
  * @note   不截断 UTF-8 中文；DMA 未完成前缓冲区保持不变（发送完成回调才释放所有权）。
  * @note   只能在任务上下文调用；串口忙时调用者应等待下次限频日志重试（不会阻塞当前任务）。
  */
bool usart_try_printf(const char *fmt, ...);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

