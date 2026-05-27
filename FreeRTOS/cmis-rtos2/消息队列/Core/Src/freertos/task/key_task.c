#include "app_msg.h"
#include "key_task.h"
#include "cmsis_os2.h"
#include "stm32f4xx_hal_def.h"
#include "usart.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern osMessageQueueId_t usartQueueHandle;

static void sendKeyMsg(const char *value) {
  AppMsg_t *msg = AppMsg_Alloc();
  if (msg == NULL) {
    return;
  }

  msg->id = (uint8_t)rand();
  msg->value = value;

  (void)AppMsg_Send(usartQueueHandle, msg, osWaitForever);
}

void StartKeyTask(void *argument) {
  for (;;) {
    switch (Key_GetEvent(argument)) {
    case KEY_EVENT_SHORT_PRESS:
      sendKeyMsg("short_press");
      SEGGER_RTT_printf(0, "short_press\r\n");
      break;
    case KEY_EVENT_LONG_PRESS:
      sendKeyMsg("long_press");
      SEGGER_RTT_printf(0, "long_press\r\n");
      break;
    default:
      break;
    }
    osDelay(10);
  }
}

void StartMsgHandleTask(void *argument) {
  AppMsg_t *recMsg = NULL;
  for (;;) {
    if (AppMsg_Receive(usartQueueHandle, &recMsg, osWaitForever) == osOK) {
      usart_printf("[id]:%d [value]:%s\r\n", recMsg->id, recMsg->value);
      AppMsg_Free(recMsg);
      recMsg = NULL;
    }
  }
}
