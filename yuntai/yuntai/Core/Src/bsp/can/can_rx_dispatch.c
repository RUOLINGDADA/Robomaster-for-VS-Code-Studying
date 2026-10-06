/**
 * @file can_rx_dispatch.c
 * @brief CAN FIFO0 的统一接收分发。
 *
 * HAL CAN ISR 只取一帧，再交给 GM6020 和 C610/M2006 按标准 ID 过滤。
 * 各驱动分别取 FIFO 会丢弃其它设备反馈。本入口不打印、不等待、不执行闭环。
 */

#include "can.h"
#include "bsp/c610_m2006/c610_m2006.h"
#include "bsp/gm6020/gm6020.h"

/**
 * @brief  处理 CAN FIFO0 pending 中断。
 * @param  hcan 产生中断的 CAN 外设。
 * @retval None。HAL 取帧失败时本次退出。
 * @note   当前 CAN1 使用 FIFO0。统一入口先取出一帧，再把同一份数据交给
 * 两个驱动，避免驱动之间争抢 FIFO 或丢失不属于自己的帧。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  CAN_RxHeaderTypeDef rx_header = {0};
  uint8_t data[8] = {0};
  if (hcan == NULL || HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, data) != HAL_OK) {
    return;
  }

  /* 先取一次帧，再把同一份数据交给各驱动。驱动自己判断这帧是不是自己的。 */
  (void)Gm6020_HandleRxMessage(hcan, &rx_header, data);
  (void)C610_M2006_HandleRxMessage(hcan, &rx_header, data);
}
