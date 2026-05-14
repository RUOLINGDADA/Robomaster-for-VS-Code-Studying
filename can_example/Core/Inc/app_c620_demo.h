#ifndef APP_C620_DEMO_H
#define APP_C620_DEMO_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

bool App_C620Demo_Init(CAN_HandleTypeDef *hcan);
void App_C620Demo_Process(void);
void App_C620Demo_OnCanRxMessage(CAN_HandleTypeDef *hcan,
                                 const CAN_RxHeaderTypeDef *rx_header,
                                 const uint8_t data[8]);

#endif /* APP_C620_DEMO_H */
