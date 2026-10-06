/**
 * @file log.c
 * @brief 共用日志的格式化、缓冲区所有权和 USART1 DMA 提交。
 *
 * 所有任务共用一个缓冲区。原子交换取得所有权，格式化期间不关闭中断。
 * DMA 完成前覆盖缓冲区会改变发送内容，因此忙时立即拒绝。模块不创建 RTOS 对象。
 * UART/DMA 初始化仍由 CubeMX 管理；本模块只使用 USART1，不处理 DBUS 接收。
 */
#include "app/log/log.h"
#include "usart.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define LOG_TX_BUFFER_SIZE 768U /* DMA 缓冲区大小，字节；最大文本长度为 767 字节。 */

#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
static uint8_t g_log_tx_buffer[LOG_TX_BUFFER_SIZE]; /* DMA 完成前不可修改的唯一缓冲区。 */
static bool g_log_tx_busy; /* 原子访问的缓冲区所有权；任务取得，DMA 回调释放。 */
#endif

/**
 * @brief 使用唯一缓冲区提交一条格式化文本。
 * @param fmt printf 格式串；空指针直接拒绝。
 * @param ... 对应格式参数。
 * @retval true DMA 接受；false 参数、上下文、长度、占用或 HAL 状态无效。
 * @note 仅任务调用。成功由回调释放；所有失败出口立即释放，不等待串口。
 */
bool Log_TryPrintf(const char *fmt, ...) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
  /* 先拒绝 ISR，再取得所有权。格式化不占临界区，也不覆盖仍在发送的数据。 */
  if (fmt == NULL || __get_IPSR() != 0U ||
      __atomic_exchange_n(&g_log_tx_busy, true, __ATOMIC_ACQUIRE)) {
    return false;
  }
  if (huart1.gState != HAL_UART_STATE_READY) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  va_list args;
  va_start(args, fmt);
  const int length = vsnprintf((char *)g_log_tx_buffer, sizeof(g_log_tx_buffer), fmt, args);
  va_end(args);
  /* 使用 int 检查格式化结果。超长文本整条拒绝，不发送被截断的中文。 */
  if (length <= 0 || length >= (int)sizeof(g_log_tx_buffer) ||
      HAL_UART_Transmit_DMA(&huart1, g_log_tx_buffer, (uint16_t)length) != HAL_OK) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
    return false;
  }
  return true;
#else
  (void)fmt;
  return false;
#endif
}

/**
 * @brief 在 USART1 DMA 完成后释放日志缓冲区。
 * @param huart HAL 完成发送的 UART 句柄；其它 UART 不影响日志所有权。
 * @retval None。
 * @note HAL ISR 回调，只释放原子标志，不格式化、不打印，也不调用 RTOS。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
#if LOG_GLOBAL_ENABLE && LOG_USART1_ENABLE
  if (huart == &huart1) {
    __atomic_store_n(&g_log_tx_busy, false, __ATOMIC_RELEASE);
  }
#else
  (void)huart;
#endif
}
