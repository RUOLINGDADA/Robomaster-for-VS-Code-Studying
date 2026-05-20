#include "SEGGER_RTT.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "key.h"
#include "task.h"

#define PORT_MASK_TEST_LOOP_COUNT 30000000UL

static void BusyWait(volatile uint32_t count) {
  while (count-- > 0U) {
    __NOP();
  }
}

static void RunPortInterruptMaskTest(void) {
  SEGGER_RTT_printf(0, "portDISABLE_INTERRUPTS\r\n");

  portDISABLE_INTERRUPTS();
  BusyWait(PORT_MASK_TEST_LOOP_COUNT);
  portENABLE_INTERRUPTS();

  SEGGER_RTT_printf(0, "portENABLE_INTERRUPTS\r\n");
}

void StartKeyTask(void *argument) {
  for (;;) {
    switch (Key_GetEvent(argument)) {
    case KEY_EVENT_SHORT_PRESS:
      RunPortInterruptMaskTest();
      break;
    case KEY_EVENT_LONG_PRESS:
      /* 用户长按功能写在这里。 */
      break;
    default:
      break;
    }
    osDelay(10);
  }
}
