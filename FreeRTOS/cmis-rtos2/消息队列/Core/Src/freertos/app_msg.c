#include "app_msg.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>
#include <string.h>

/*
 * 内存池
 * APP_MSG_POOL_SIZE 同时作为内存池大小和消息队列长度使用。
 * 这里静态定义的全局数组位于 .bss 段，程序正常复位启动时会被启动文件清零。
 */
static AppMsg_t Msg_Pool[APP_MSG_POOL_SIZE];
static uint8_t Used_Msg_Pool[APP_MSG_POOL_SIZE];

void AppMsg_Init(void) {
  memset(Msg_Pool, 0, sizeof(Msg_Pool));
  memset(Used_Msg_Pool, 0, sizeof(Used_Msg_Pool));
}

// 从内存池中分配一个 AppMsg_t 地址，返回 NULL 表示内存池已满
AppMsg_t *AppMsg_Alloc(void) {
  taskENTER_CRITICAL();
  for (uint8_t i = 0; i < APP_MSG_POOL_SIZE; i++) {
    if (!Used_Msg_Pool[i]) {
      Used_Msg_Pool[i] = 1;
      taskEXIT_CRITICAL();
      return &Msg_Pool[i];
    }
  }
  taskEXIT_CRITICAL();
  return NULL;
}

// 释放 msg 对应的内存池槽位，把使用标志位清零
void AppMsg_Free(AppMsg_t *msg) {
  if (msg == NULL) {
    return;
  }

  /*
   * 指针相减的结果是“相差多少个元素”，不是相差多少字节。
   * 例如 msg == &Msg_Pool[3] 时，msg - Msg_Pool 的结果就是 3。
   */
  ptrdiff_t idx = msg - Msg_Pool;
  if (idx < 0 || idx >= (ptrdiff_t)APP_MSG_POOL_SIZE) {
    return;
  }

  taskENTER_CRITICAL();
  memset(msg, 0, sizeof(*msg));
  Used_Msg_Pool[idx] = 0;
  taskEXIT_CRITICAL();
}

osStatus_t AppMsg_Send(osMessageQueueId_t queue, AppMsg_t *msg,
                       uint32_t timeout) {
  if (queue == NULL || msg == NULL) {
    return osErrorParameter;
  }

  /*
   * 队列元素类型是 AppMsg_t *。
   * osMessageQueuePut 的 msg_ptr 参数要求传“待拷贝对象的地址”。
   * 这里待拷贝对象是指针变量 msg 本身，所以传 &msg，而不是 msg。
   */
  osStatus_t status = osMessageQueuePut(queue, &msg, 0, timeout);
  if (status != osOK) {
    AppMsg_Free(msg);
  }

  return status;
}

osStatus_t AppMsg_Receive(osMessageQueueId_t queue, AppMsg_t **msg,
                          uint32_t timeout) {
  if (queue == NULL || msg == NULL) {
    return osErrorParameter;
  }

  *msg = NULL;
  /*
   * 接收的队列元素也是 AppMsg_t *。
   * osMessageQueueGet 需要把取出的指针值写入 *msg，所以这里传 msg。
   * 调用者传入的是 &recMsg，因此这里的 msg 本质上仍是“指针变量的地址”。
   */
  return osMessageQueueGet(queue, msg, NULL, timeout);
}
