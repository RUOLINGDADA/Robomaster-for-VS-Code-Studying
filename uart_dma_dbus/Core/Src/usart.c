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
#define DEBUG_RAW_ENABLE  0   // 调试完可改为 0 关闭

#if DEBUG_RAW_ENABLE
static volatile uint8_t debug_raw_buf[18];
static volatile uint16_t debug_raw_size = 0;
static volatile uint8_t debug_raw_ready = 0;
#endif
uint8_t sbus_rx_buf[2][36u];
static volatile uint8_t buf_idx = 0;
static volatile uint8_t uart3_error_flag = 0;  // 错误标志（中断只置位，主循环打印）
static volatile uint32_t uart3_err_code = 0;
static volatile uint32_t dma3_err_code = 0;
SBUS_RemoteControl_t g_remote_control;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart3_rx;

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
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

    /* USER CODE BEGIN USART3_Init 0 */

    /* USER CODE END USART3_Init 0 */

    /* USER CODE BEGIN USART3_Init 1 */

    /* USER CODE END USART3_Init 1 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 100000;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_EVEN;
    huart3.Init.Mode = UART_MODE_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART3_Init 2 */

    /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (uartHandle->Instance == USART1)
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
        hdma_usart1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart1_tx);

        /* USART1_RX Init */
        hdma_usart1_rx.Instance = DMA2_Stream2;
        hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_NORMAL;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspInit 1 */

        /* USER CODE END USART1_MspInit 1 */
    }
    else if (uartHandle->Instance == USART3)
    {
        /* USER CODE BEGIN USART3_MspInit 0 */

        /* USER CODE END USART3_MspInit 0 */
        /* USART3 clock enable */
        __HAL_RCC_USART3_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**USART3 GPIO Configuration
        PC11     ------> USART3_RX
        PC10     ------> USART3_TX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USART3 DMA Init */
        /* USART3_RX Init */
        hdma_usart3_rx.Instance = DMA1_Stream1;
        hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_rx.Init.Mode = DMA_NORMAL;
        hdma_usart3_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart3_rx);

        /* USART3 interrupt Init */
        HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
        /* USER CODE BEGIN USART3_MspInit 1 */

        /* USER CODE END USART3_MspInit 1 */
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

    if (uartHandle->Instance == USART1)
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
        HAL_DMA_DeInit(uartHandle->hdmarx);

        /* USART1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspDeInit 1 */

        /* USER CODE END USART1_MspDeInit 1 */
    }
    else if (uartHandle->Instance == USART3)
    {
        /* USER CODE BEGIN USART3_MspDeInit 0 */

        /* USER CODE END USART3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART3_CLK_DISABLE();

        /**USART3 GPIO Configuration
        PC11     ------> USART3_RX
        PC10     ------> USART3_TX
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_11 | GPIO_PIN_10);

        /* USART3 DMA DeInit */
        HAL_DMA_DeInit(uartHandle->hdmarx);

        /* USART3 interrupt Deinit */
        HAL_NVIC_DisableIRQ(USART3_IRQn);
        /* USER CODE BEGIN USART3_MspDeInit 1 */

        /* USER CODE END USART3_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
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

    //HAL_UART_Transmit(&huart1, tx_buf, len, 99999999);
    // 启动DMA传输
    if (HAL_UART_Transmit_DMA(&huart1, tx_buf, len) != HAL_OK)
    {
        Error_Handler();
    }
}

void RemoteControl_Init(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sbus_rx_buf[buf_idx], 36u);
}

static void SBUS_DecodeToRemoteData(const volatile uint8_t *sbus_raw_data, SBUS_RemoteControl_t *remote_data)
{
    if (sbus_raw_data == NULL || remote_data == NULL)
    {
        return;
    }
    remote_data->controller.channels[0] = (sbus_raw_data[0] | (sbus_raw_data[1] << 8)) & 0x07FF;
    remote_data->controller.channels[1] = ((sbus_raw_data[1] >> 3) | (sbus_raw_data[2] << 5)) & 0x07FF;
    remote_data->controller.channels[2] = ((sbus_raw_data[2] >> 6) | (sbus_raw_data[3] << 2) | (sbus_raw_data[4] << 10)) & 0x07FF;
    remote_data->controller.channels[3] = ((sbus_raw_data[4] >> 1) | (sbus_raw_data[5] << 7)) & 0x07FF;

    uint8_t sw = (sbus_raw_data[5] >> 4) & 0x0F;
    remote_data->controller.switches[0] = (sw >> 2) & 0x03; // S1
    remote_data->controller.switches[1] = sw & 0x03;        // S2

    remote_data->mouse.x = sbus_raw_data[6] | (sbus_raw_data[7] << 8);
    remote_data->mouse.y = sbus_raw_data[8] | (sbus_raw_data[9] << 8);
    remote_data->mouse.z = sbus_raw_data[10] | (sbus_raw_data[11] << 8);
    remote_data->mouse.press_left = sbus_raw_data[12];
    remote_data->mouse.press_right = sbus_raw_data[13];
    remote_data->keyboard.keys = sbus_raw_data[14] | (sbus_raw_data[15] << 8);
    remote_data->controller.channels[4] = sbus_raw_data[16] | (sbus_raw_data[17] << 8);

    remote_data->controller.channels[0] -= 1024;
    remote_data->controller.channels[1] -= 1024;
    remote_data->controller.channels[2] -= 1024;
    remote_data->controller.channels[3] -= 1024;
    remote_data->controller.channels[4] -= 1024;
}

// UART 空闲中断事件回调
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART3)
    {
#if DEBUG_RAW_ENABLE
        // 安全拷贝原始数据（中断内只存，不打印）
        for (int i = 0; i < Size && i < 18; i++)
        {
            debug_raw_buf[i] = sbus_rx_buf[buf_idx][i];
        }
        debug_raw_size = Size;
        debug_raw_ready = 1;
#endif

        if (Size == 18u)
        {
            // 调用解码函数，解码当前缓冲区的数据
            SBUS_DecodeToRemoteData(sbus_rx_buf[buf_idx], &g_remote_control);
        }
        // 切换缓冲区索引
        buf_idx = (buf_idx + 1) % 2;

        // 再次启动接收，准备下一帧
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sbus_rx_buf[buf_idx], 36u);
    }
}

void USART3_Debug_PrintRaw(void)
{
#if DEBUG_RAW_ENABLE
    if (debug_raw_ready)
    {
        usart_printf("Raw[%d]: ", debug_raw_size);
        for (int i = 0; i < debug_raw_size && i < 18; i++)
        {
            usart_printf("%02X ", debug_raw_buf[i]);
        }
        usart_printf("\r\n");
        debug_raw_ready = 0;   // 清除标志
    }
#endif
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // 只存错误码，主循环处理
        uart3_err_code = HAL_UART_GetError(huart);
        dma3_err_code = hdma_usart3_rx.ErrorCode;
        uart3_error_flag = 1;

        // 清除所有错误标志（必须全清，否则锁死）
        __HAL_UART_CLEAR_PEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_OREFLAG(huart);

        // 停止DMA+重启（安全重启）
        HAL_UART_AbortReceive(huart);
        buf_idx = (buf_idx + 1) % 2;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, sbus_rx_buf[buf_idx], 36u);
    }
}

void USART3_Check_Error(void)
{
    if (uart3_error_flag)
    {
        usart_printf("[FATAL] UART3 ERR: 0x%08lX | DMA ERR: 0x%08lX\r\n", uart3_err_code, dma3_err_code);
        uart3_error_flag = 0; // 清除标志
    }
}
/* USER CODE END 1 */

