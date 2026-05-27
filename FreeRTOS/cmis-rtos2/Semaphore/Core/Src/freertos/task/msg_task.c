#include "FreeRTOS.h"
#include "app_msg.h"
#include "cmsis_os2.h"
#include "main.h"
void StartMsgHandleTask(void *argument) {
  AppMsg_t *recMsg = NULL;
  for (;;) {
    if (AppMsg_Receive(usartQueueHandle, &recMsg, osWaitForever) == osOK) {
      usart_printf("[id]:%d [name]:%s [value]:%s [资源剩余]:%ld\r\n",
                   recMsg->id, recMsg->name, recMsg->value,
                   osSemaphoreGetCount(cryptoChipHandle));
      AppMsg_Free(recMsg);
      recMsg = NULL;
    }
  }
}