#ifndef __CAN_EX_H
#define __CAN_EX_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal_can.h"
#include <stdlib.h>

// 返回状态
typedef enum {
  CAN_OK,
  CAN_ERROR,
  CAN_ERROR_TIMEOUT,
  CAN_ERROR_EMPTY,
  CAN_ERROR_NULL_PARAM
} CAN_StatusTypeDef;

// 平台枚举
typedef enum { CAN_PLATFORM_STM32F4 } CAN_PlatformTypeDef;

// 拓展CAN.c句柄结构体(不透明结构体)
typedef struct CAN_HandleTypeDefEx CAN_HandleTypeDefEx;

// 构造函数
CAN_HandleTypeDefEx *CAN_Create(CAN_HandleTypeDef *hcan,
                                CAN_PlatformTypeDef platform);
// 析构函数
void CAN_Destroy(CAN_HandleTypeDefEx *hcan);

/**
 * @brief 发送CAN报文
 * @param hcan CAN对象指针
 * @param tx_header 发送报文头
 * @param data 发送数据（最多8字节）
 * @param tx_mailbox 发送邮箱号（输出参数）
 * @retval CAN状态
 */
CAN_StatusTypeDef CAN_SendData(CAN_HandleTypeDefEx *hcan,
                               CAN_TxHeaderTypeDef *tx_header, uint8_t *data,
                               uint32_t *tx_mailbox);

/**
 * @brief 接收CAN报文
 * @param hcan CAN对象指针
 * @param fifo 接收FIFO（CAN_RX_FIFO0/CAN_RX_FIFO1）
 * @param rx_header 接收报文头
 * @param data 接收数据缓冲区（至少8字节）
 * @retval CAN状态
 */
CAN_StatusTypeDef CAN_ReceiveData(CAN_HandleTypeDefEx *hcan, uint32_t fifo,
                                  CAN_RxHeaderTypeDef *rx_header,
                                  uint8_t *data);

CAN_StatusTypeDef CAN_EnableInterrupt(CAN_HandleTypeDefEx *hcan,
                                      uint32_t it_flags);

#endif /* __CAN_EX_H */