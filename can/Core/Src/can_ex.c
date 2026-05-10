#include "can_ex.h"

// 函数指针
typedef void (*CAN_DeInit_Func)(CAN_HandleTypeDefEx *hcan);
typedef CAN_StatusTypeDef (*CAN_WriteFilter_Func)(CAN_HandleTypeDefEx *hcan,
                                                  CAN_FilterTypeDef *filter);
typedef CAN_StatusTypeDef (*CAN_SendMsg_Func)(CAN_HandleTypeDefEx *hcan,
                                              CAN_TxHeaderTypeDef *tx_header,
                                              uint8_t *data,
                                              uint32_t *tx_mailbox);
typedef CAN_StatusTypeDef (*CAN_ReceiveMsg_Func)(CAN_HandleTypeDefEx *hcan,
                                                 uint32_t fifo,
                                                 CAN_RxHeaderTypeDef *rx_header,
                                                 uint8_t *data);
typedef CAN_StatusTypeDef (*CAN_EnableIT_Func)(CAN_HandleTypeDefEx *hcan,
                                               uint32_t it_flags);

struct CAN_HandleTypeDefEx {
  CAN_HandleTypeDef *hcan;
  CAN_PlatformTypeDef Platform;

  // 错误码缓存
  uint32_t ErrorCode;

  // 函数指针表(虚函数表 实现多态关键)
  CAN_DeInit_Func deinit;
  CAN_WriteFilter_Func write_filter;
  CAN_SendMsg_Func sendmsg;
  CAN_ReceiveMsg_Func receive_msg;
  CAN_EnableIT_Func enable_it;
};

/* 底层实现函数声明(内部使用) */
// STM32F4
static void CAN_DeInit_F4(CAN_HandleTypeDefEx *hcan);
static CAN_StatusTypeDef CAN_WriteFilter_F4(CAN_HandleTypeDefEx *hcan,
                                            CAN_FilterTypeDef *filter);
static CAN_StatusTypeDef CAN_SendMsg_F4(CAN_HandleTypeDefEx *hcan,
                                        CAN_TxHeaderTypeDef *tx_header,
                                        uint8_t *data, uint32_t *tx_mailbox);
static CAN_StatusTypeDef CAN_ReceiveMsg_F4(CAN_HandleTypeDefEx *hcan,
                                           uint32_t fifo,
                                           CAN_RxHeaderTypeDef *rx_header,
                                           uint8_t *data);
static CAN_StatusTypeDef CAN_EnableIT_F4(CAN_HandleTypeDefEx *hcan,
                                         uint32_t it_flags);

// 构造函数
CAN_HandleTypeDefEx *CAN_Create(CAN_HandleTypeDef *hcan,
                                CAN_PlatformTypeDef platform) {
  if (hcan == NULL) {
    return NULL;
  }

  // 堆分配内存(注意:C++ 不允许void*隐式转换为其他指针类型)
  CAN_HandleTypeDefEx *hcan_ex =
      (CAN_HandleTypeDefEx *)malloc(sizeof(CAN_HandleTypeDefEx));
  // 检查有没有分配成功
  if (hcan_ex == NULL) {
    return NULL;
  }

  hcan_ex->hcan = hcan;
  hcan_ex->Platform = platform;
  hcan_ex->ErrorCode = 0;

  // 根据平台参数绑定函数指针(多态)
  switch (platform) {
  case CAN_PLATFORM_STM32F4:
    hcan_ex->deinit = CAN_DeInit_F4;
    hcan_ex->write_filter = CAN_WriteFilter_F4;
    hcan_ex->sendmsg = CAN_SendMsg_F4;
    hcan_ex->receive_msg = CAN_ReceiveMsg_F4;
    hcan_ex->enable_it = CAN_EnableIT_F4;
    break;
  default:
    free(hcan_ex);
    return NULL;
  }

  // 配置默认Filter(接收所有报文)
  CAN_FilterTypeDef default_filter = {0};
  default_filter.FilterActivation = ENABLE;
  default_filter.SlaveStartFilterBank = 14; // 全局配置
  default_filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  default_filter.FilterBank = 0;
  default_filter.FilterIdHigh = 0x0000;
  default_filter.FilterIdLow = 0x0000;
  default_filter.FilterMaskIdHigh = 0x0000;
  default_filter.FilterMaskIdLow = 0x0000;
  default_filter.FilterMode = CAN_FILTERMODE_IDMASK;
  default_filter.FilterScale = CAN_FILTERSCALE_32BIT;

  if (hcan_ex->write_filter(hcan_ex, &default_filter) != CAN_OK) {
    free(hcan_ex);
    return NULL;
  }

  // 启动CAN外设(CubeMX只初始化，不启动)
  if (HAL_CAN_Start(hcan_ex->hcan) != HAL_OK) {
    hcan_ex->ErrorCode = HAL_CAN_GetError(hcan_ex->hcan);
    free(hcan_ex);
    return NULL;
  }
  return hcan_ex;
}

// 析构函数
void CAN_Destroy(CAN_HandleTypeDefEx *hcan) {
  if (hcan == NULL) {
    return;
  }

  // 调用平台相关的反初始化函数
  hcan->deinit(hcan);
  // 释放动态内存
  free(hcan);
}

/* 上层接口 */
CAN_StatusTypeDef CAN_ConfigFilter(CAN_HandleTypeDefEx *hcan,
                                   CAN_FilterTypeDef *filter) {
  if (hcan == NULL || filter == NULL) {
    return CAN_ERROR_NULL_PARAM;
  }

  return hcan->write_filter(hcan, filter);
}

CAN_StatusTypeDef CAN_SendData(CAN_HandleTypeDefEx *hcan,
                               CAN_TxHeaderTypeDef *tx_header, uint8_t *data,
                               uint32_t *tx_mailbox) {
  if (hcan == NULL || tx_header == NULL || data == NULL || tx_mailbox == NULL) {
    return CAN_ERROR_NULL_PARAM;
  }

  return hcan->sendmsg(hcan, tx_header, data, tx_mailbox);
}

CAN_StatusTypeDef CAN_ReceiveData(CAN_HandleTypeDefEx *hcan, uint32_t fifo,
                                  CAN_RxHeaderTypeDef *rx_header,
                                  uint8_t *data) {
  if (hcan == NULL || rx_header == NULL || data == NULL) {
    return CAN_ERROR_NULL_PARAM;
  }

  return hcan->receive_msg(hcan, fifo, rx_header, data);
}

CAN_StatusTypeDef CAN_EnableInterrupt(CAN_HandleTypeDefEx *hcan,
                                      uint32_t it_flags) {
  if (hcan == NULL) {
    return CAN_ERROR_NULL_PARAM;
  }

  return hcan->enable_it(hcan, it_flags);
}
/* STM32F4底层函数实现 */
static void CAN_DeInit_F4(CAN_HandleTypeDefEx *hcan) {
  // 禁用所有中断
  HAL_CAN_DeactivateNotification(hcan->hcan, CAN_IT_RX_FIFO0_MSG_PENDING |
                                                 CAN_IT_ERROR | CAN_IT_BUSOFF);

  // 停止CAN外设
  HAL_CAN_Stop(hcan->hcan);
}

static CAN_StatusTypeDef CAN_WriteFilter_F4(CAN_HandleTypeDefEx *hcan,
                                            CAN_FilterTypeDef *filter) {
  if (HAL_CAN_ConfigFilter(hcan->hcan, filter) != HAL_OK) {
    hcan->ErrorCode = HAL_CAN_GetError(hcan->hcan);
    return CAN_ERROR;
  }
  return CAN_OK;
}

static CAN_StatusTypeDef CAN_SendMsg_F4(CAN_HandleTypeDefEx *hcan,
                                        CAN_TxHeaderTypeDef *tx_header,
                                        uint8_t *data, uint32_t *tx_mailbox) {
  if (HAL_CAN_AddTxMessage(hcan->hcan, tx_header, data, tx_mailbox) != HAL_OK) {
    hcan->ErrorCode = HAL_CAN_GetError(hcan->hcan);
    return CAN_ERROR;
  }

  uint32_t timeout = 100000;
  while (HAL_CAN_GetTxMailboxesFreeLevel(hcan->hcan) != 3 && timeout--) {
  }

  if (timeout == 0) {
    return CAN_ERROR_TIMEOUT;
  }

  return CAN_OK;
}

static CAN_StatusTypeDef CAN_ReceiveMsg_F4(CAN_HandleTypeDefEx *hcan,
                                           uint32_t fifo,
                                           CAN_RxHeaderTypeDef *rx_header,
                                           uint8_t *data) {
  if (HAL_CAN_GetRxFifoFillLevel(hcan->hcan, fifo) == 0) {
    return CAN_ERROR_EMPTY;
  }

  if (HAL_CAN_GetRxMessage(hcan->hcan, fifo, rx_header, data) != HAL_OK) {
    hcan->ErrorCode = HAL_CAN_GetError(hcan->hcan);
    return CAN_ERROR;
  }

  return CAN_OK;
}

static CAN_StatusTypeDef CAN_EnableIT_F4(CAN_HandleTypeDefEx *hcan,
                                         uint32_t it_flags) {
  if (HAL_CAN_ActivateNotification(hcan->hcan, it_flags) != HAL_OK) {
    hcan->ErrorCode = HAL_CAN_GetError(hcan->hcan);
    return CAN_ERROR;
  }
  return CAN_OK;
}