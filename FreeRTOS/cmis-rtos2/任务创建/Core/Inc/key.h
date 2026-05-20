#ifndef __KEY_H
#define __KEY_H
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  KEY_OK = 0,
  KEY_ERROR,
  KEY_NO_PARAMETER,
  KEY_DEVICE_NORMAL_LOOP
} KEY_StatusTypeDef;
typedef enum { STM32F407 = 0 } KEY_PlatformTypeDef;
typedef struct KEY_HandleTypeDef KEY_HandleTypeDef; // 声明不透明结构体
// 构造函数
KEY_HandleTypeDef *Key_Create(GPIO_TypeDef *key_port, uint16_t key_pin,
                              KEY_PlatformTypeDef platform,
                              GPIO_TypeDef *led_pin_port, uint16_t led_pin);
// 析构函数
KEY_StatusTypeDef Key_Destroy(KEY_HandleTypeDef *hkey);

// 接口
KEY_StatusTypeDef Key_Device_Process_Loop(KEY_HandleTypeDef *hkey);
KEY_StatusTypeDef LED_Update_Loop(KEY_HandleTypeDef *hkey);
uint8_t Get_Device_PowerState(KEY_HandleTypeDef *hkey);
#endif