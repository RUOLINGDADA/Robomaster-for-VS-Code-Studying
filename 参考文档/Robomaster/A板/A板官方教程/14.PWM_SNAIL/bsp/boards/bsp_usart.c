#include "bsp_usart.h"
#include <stdio.h>
#include <stdarg.h>
#include "main.h"


extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;


void usart1_tx_dma_init(void)
{
    //enable the DMA transfer for the receiver request
    SET_BIT(huart1.Instance->CR3, USART_CR3_DMAT);
}


void usart1_tx_dma_enable(uint8_t *data, uint16_t len)
{

    //disable DMA
    __HAL_DMA_DISABLE(&hdma_usart1_tx);
    while(hdma_usart1_tx.Instance->CR & DMA_SxCR_EN)
    {
        __HAL_DMA_DISABLE(&hdma_usart1_tx);
    }

    //clear flag
    __HAL_DMA_CLEAR_FLAG(&hdma_usart1_tx, DMA_HISR_TCIF7);
    __HAL_DMA_CLEAR_FLAG(&hdma_usart1_tx, DMA_HISR_HTIF7);

    //set data address
    hdma_usart1_tx.Instance->M0AR = (uint32_t)(data);
    //set data length
    hdma_usart1_tx.Instance->NDTR = len;

    //enable DMA
    __HAL_DMA_ENABLE(&hdma_usart1_tx);
}


void usart_printf(const char *fmt,...)
{
    static uint8_t tx_buf[256] = {0};
    static va_list ap;
    static uint16_t len;
	
	while (huart1.gState != HAL_UART_STATE_READY)
	{
        HAL_Delay(1); // 短暂延迟，避免忙等待
    }
	
    va_start(ap, fmt);

    len = vsprintf((char *)tx_buf, fmt, ap);

    va_end(ap);

//    usart1_tx_dma_enable(tx_buf, len);
//	
//	HAL_UART_Transmit_DMA(&huart1, tx_buf, len);
	
	HAL_UART_Transmit(&huart1, tx_buf, len, 1000); // 1秒超时
	
}

