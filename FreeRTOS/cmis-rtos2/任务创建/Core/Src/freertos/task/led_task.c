#include "cmsis_os2.h"
#include "main.h"

void Start_LED_Flash_Task(void *argument) {
  (void)argument;
  for (;;) {
    HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
    osDelay(500);
  }
}