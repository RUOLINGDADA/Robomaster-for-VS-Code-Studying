/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
/**
 * @brief  SBUS 遥控器数据结构体
 * @note   对应大疆 DT7 遥控器 + DR16 接收机
 */
typedef struct
{
    /**
     * @brief  遥控器摇杆与拨杆数据
     */
    struct
    {
        int16_t channels[5];  /*!< 摇杆通道值 (CH0-CH4)
                                    - 范围: -1024 ~ +1024
                                    - CH0: 右侧摇杆左右
                                    - CH1: 右侧摇杆上下
                                    - CH2: 左侧摇杆上下
                                    - CH3: 左侧摇杆左右
                                    - CH4: 保留 */

        uint8_t switches[2];   /*!< 拨杆开关值 (S1, S2)
                                    - 1: 上
                                    - 2: 中
                                    - 3: 下 */
    } controller;

    /**
     * @brief  鼠标数据 (如果连接了鼠标)
     */
    struct
    {
        int16_t x;              /*!< 鼠标 X 轴移动量 */
        int16_t y;              /*!< 鼠标 Y 轴移动量 */
        int16_t z;              /*!< 鼠标 Z 轴 (滚轮) 移动量 */
        uint8_t press_left;     /*!< 鼠标左键按下状态 (0: 松开, 1: 按下) */
        uint8_t press_right;    /*!< 鼠标右键按下状态 (0: 松开, 1: 按下) */
    } mouse;

    /**
     * @brief  键盘数据 (如果连接了键盘)
     */
    struct
    {
        uint16_t keys;          /*!< 键盘按键位域 (每一位代表一个按键) */
    } keyboard;

} SBUS_RemoteControl_t;

// 全局遥控器数据 (声明)
extern SBUS_RemoteControl_t g_remote_control;
extern uint8_t sbus_rx_buf[2][36u];

// 函数声明
void RemoteControl_Init(void);
void usart_printf(const char *fmt, ...);
void USART3_Check_Error(void);
void USART3_Debug_PrintRaw(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

