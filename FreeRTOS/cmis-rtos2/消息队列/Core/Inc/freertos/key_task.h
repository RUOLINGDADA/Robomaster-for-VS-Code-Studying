#ifndef __KEY_TASK_H
#define __KEY_TASK_H
#include "FreeRTOS.h"
#include "SEGGER_RTT.h"
#include "cmsis_os2.h"
#include "key.h"
#include "stm32f407xx.h"
#include "task.h"
#include "usart.h"
#include <stdlib.h>

typedef struct {
  uint8_t id;
  char *value;
} Msg_t;

#endif