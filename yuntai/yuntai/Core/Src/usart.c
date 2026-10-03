/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

#define USART_TX_BUFFER_SIZE 768U /* 中文诊断文本 DMA 缓冲区大小，单位字节。 */

static uint8_t g_usart_tx_buf[USART_TX_BUFFER_SIZE]; /* DMA 发送期间保持不变的唯一共享缓冲区。 */
static volatile bool g_usart_tx_busy; /* DMA 未完成时为 true，避免覆盖发送内容。 */

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/*
 * 原子交换只保护“缓冲区所有权”，不会在 vsnprintf 期间关闭中断。
 * DMA 正在读取时所有任务立即返回 false；DMA 完成后才可以覆盖缓冲区。
 * 格式化返回的是 int，超过缓冲区则整条拒绝，不截断 UTF-8 中文字符。
 */
static bool Usart_TryVPrintf(const char *fmt, va_list ap) {
  if (fmt == NULL || __get_IPSR() != 0U ||
      __atomic_exchange_n(&g_usart_tx_busy, true, __ATOMIC_ACQUIRE)) {
    return false;
  }
  if (huart1.gState != HAL_UART_STATE_READY) {
    __atomic_store_n(&g_usart_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  const int length = vsnprintf((char *)g_usart_tx_buf,
                              sizeof(g_usart_tx_buf), fmt, ap);
  if (length <= 0 || length >= (int)sizeof(g_usart_tx_buf)) {
    __atomic_store_n(&g_usart_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  if (HAL_UART_Transmit_DMA(&huart1, g_usart_tx_buf,
                           (uint16_t)length) != HAL_OK) {
    __atomic_store_n(&g_usart_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  return true; /* 所有权保留到 DMA 完成回调；不能在这里提前释放。 */
}

bool usart_try_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const bool accepted = Usart_TryVPrintf(fmt, ap);
  va_end(ap);
  return accepted;
}

void usart_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  (void)Usart_TryVPrintf(fmt, ap); /* 旧 void 接口忙时丢弃；重要提示使用 try 接口重试。 */
  va_end(ap);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    __atomic_store_n(&g_usart_tx_busy, false, __ATOMIC_RELEASE);
  }
}
/* USER CODE END 1 */
