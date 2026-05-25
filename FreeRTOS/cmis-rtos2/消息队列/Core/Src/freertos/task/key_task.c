#include "key_task.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_def.h"
#include "usart.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*内存池*/
#define MSG_POOL_SIZE 16 // 内存池大小(队列长度)
static Msg_t Msg_Pool[MSG_POOL_SIZE];
static uint8_t Used_Msg_Pool[MSG_POOL_SIZE]; // 这里声明的变量会存在.bss段
                                             // 程序复位会清零该数组
extern osMessageQueueId_t usartQueueHandle;

//  内存池中分配msg地址
static Msg_t *allocMsg() {
  taskENTER_CRITICAL();
  for (uint8_t i = 0; i < MSG_POOL_SIZE; i++) {
    if (!Used_Msg_Pool[i]) {
      Used_Msg_Pool[i] = 1;
      taskEXIT_CRITICAL();
      return &Msg_Pool[i];
    }
  }
  taskEXIT_CRITICAL();
  return NULL;
}

// msg标志位清零
static void freeMsg(Msg_t *msg_pool) {
  if (!msg_pool) {
    return;
  }
  taskENTER_CRITICAL();
  /*int8_t idx = (int8_t)(msg_pool - Msg_Pool);
    p -
    q的结果是“相差多少个元素”(指针表示指向的元素p+1对应指向下一个元素)，不是相差多少字节
        uintptr_t byte_diff_1 = (uintptr_t)((uint8_t *)p - (uint8_t *)q); //
    字节差 uintptr_t byte_diff_2 = (uintptr_t)((char *)p - (char *)q);       //
    等价写法
  */
  ptrdiff_t idx = msg_pool - Msg_Pool; // 获得当前msg地址在内存池的索引
                                       // 这里ptrdiff_t为指针差值类型更安全
  if (idx >= 0 && idx < MSG_POOL_SIZE) {
    Used_Msg_Pool[idx] = 0;
  }
  taskEXIT_CRITICAL();
}

void StartKeyTask(void *argument) {
  for (;;) {
    switch (Key_GetEvent(argument)) {
    case KEY_EVENT_SHORT_PRESS:
      Msg_t *msg = allocMsg();
      if (msg != NULL) {
        msg->id = (uint8_t)rand();
        msg->value = "short_press";
        // msg_ptr 要求传的是变量的地址 这里是指针变量(对应内存池中各个指针)
        // 所以是指针的地址
        if (osMessageQueuePut(usartQueueHandle, &msg, 0, osWaitForever) !=
            osOK) {
          freeMsg(msg);
        }
        break;
      }
      break;
    case KEY_EVENT_LONG_PRESS:

      break;
    default:
      break;
    }
    osDelay(10);
  }
}

void StartMsgHandleTask(void *argument) {
  Msg_t *recMsg = NULL;
  for (;;) {
    // 接收的是指针变量 所以这里传的是指针变量的地址
    if (osMessageQueueGet(usartQueueHandle, &recMsg, NULL, osWaitForever) ==
        osOK) {
      usart_printf("[id]:%d [value]:%s\r\n", recMsg->id, recMsg->value);
      freeMsg(recMsg);
    };
  }
}