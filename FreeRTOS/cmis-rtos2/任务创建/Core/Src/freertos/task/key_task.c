#include "key.h"

void StartKeyTask(void *argument) {
  for (;;) {
    Key_Device_Process_Loop(argument);
    if (Get_Device_PowerState(argument)) {
      LED_Update_Loop(argument);
    }
  }
}