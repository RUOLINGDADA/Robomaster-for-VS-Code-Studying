#include "FreeRTOS.h"
#include "app_msg.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"
#include <stdlib.h>

static void sendTaskMsg(const char *value) {
  AppMsg_t *msg = AppMsg_Alloc();
  if (msg == NULL) {
    return;
  }

  msg->id = (uint8_t)rand();
  msg->name = "crypto";
  msg->value = value;

  (void)AppMsg_Send(usartQueueHandle, msg, 0);
}

void StartTask01(void *argument) {
  for (;;) {
    if (osSemaphoreAcquire(cryptoChipHandle, osWaitForever) == osOK) {
      sendTaskMsg("执行任务01");
      osDelay(2000);
      osSemaphoreRelease(cryptoChipHandle);
    }
    osDelay(2000);
  }
}

void StartTask02(void *argument) {
  for (;;) {
    if (osSemaphoreAcquire(cryptoChipHandle, osWaitForever) == osOK) {
      sendTaskMsg("执行任务02");
      osDelay(2000);
      osSemaphoreRelease(cryptoChipHandle);
    }
    osDelay(3000);
  }
}

void StartTask03(void *argument) {
  for (;;) {
    if (osSemaphoreAcquire(cryptoChipHandle, osWaitForever) == osOK) {
      sendTaskMsg("执行任务03");
      osDelay(2000);
      osSemaphoreRelease(cryptoChipHandle);
    }
    osDelay(4000);
  }
}