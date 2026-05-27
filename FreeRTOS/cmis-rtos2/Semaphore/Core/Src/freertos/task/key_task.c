#include "key_task.h"
#include "SEGGER_RTT.h"
#include "cmsis_os.h"
#include "key.h"

static void sendKeyMsg(const char *value) {
  AppMsg_t *msg = AppMsg_Alloc();
  if (msg == NULL) {
    return;
  }

  msg->id = (uint8_t)rand();
  msg->name = "Key";
  msg->value = value;

  (void)AppMsg_Send(usartQueueHandle, msg, 0);
}

void StartKeyTask(void *argument) {
  for (;;) {
    if (osSemaphoreAcquire(KeyBinarySemHandle, osWaitForever) == osOK) {
      if (osSemaphoreAcquire(cryptoChipHandle, osWaitForever) == osOK) {
        uint32_t start = osKernelGetTickCount();
        KEY_EventTypeDef event = KEY_EVENT_NONE;
        while ((osKernelGetTickCount() - start) < 1500) {
          event = Key_GetEvent(argument);
          if (event != KEY_EVENT_NONE) {
            break;
          }
          osDelay(10);
        }
        SEGGER_RTT_printf(0, "get sem\r\n");
        switch (event) {
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
        osSemaphoreRelease(cryptoChipHandle);
      }
    }
    osDelay(1);
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  osSemaphoreRelease(KeyBinarySemHandle);
}