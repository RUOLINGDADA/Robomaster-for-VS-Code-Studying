#ifndef __KEY_H
#define __KEY_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  KEY_OK = 0,
  KEY_ERROR,
  KEY_NO_PARAMETER
} KEY_StatusTypeDef;

typedef enum {
  KEY_EVENT_NONE = 0,
  KEY_EVENT_SHORT_PRESS,
  KEY_EVENT_LONG_PRESS
} KEY_EventTypeDef;

typedef struct KEY_HandleTypeDef KEY_HandleTypeDef;

KEY_HandleTypeDef *Key_Create(GPIO_TypeDef *key_port, uint16_t key_pin);
KEY_StatusTypeDef Key_Destroy(KEY_HandleTypeDef *hkey);

KEY_EventTypeDef Key_GetEvent(KEY_HandleTypeDef *hkey);
bool Key_IsTriggered(KEY_HandleTypeDef *hkey);

#endif
