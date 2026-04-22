#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/************************ 硬件配置 ************************/
#define OLED_I2C_ADDR        0x78    // OLED I2C地址
#define OLED_I2C_HANDLE      hi2c2  // 对应开发板I2C2
#define OLED_WIDTH           128    // 屏幕宽度
#define OLED_HEIGHT          64     // 屏幕高度

/************************ 命令/数据标识 ************************/
#define OLED_CMD             0x00    // 写命令
#define OLED_DATA            0x40    // 写数据

/************************ 函数声明 ************************/
/**
 * @brief  OLED初始化
 * @retval 无
 */
void OLED_Init(void);

/**
 * @brief  OLED清屏（全黑）
 * @retval 无
 */
void OLED_Clear(void);

/**
 * @brief  在指定坐标画点
 * @param  x: X坐标(0~127)
 * @param  y: Y坐标(0~63)
 * @retval 无
 */
void OLED_DrawPoint(uint8_t x, uint8_t y);

/**
 * @brief  显示单个ASCII字符
 * @param  x: 起始X坐标(0~127)
 * @param  y: 起始Y坐标(0~63，8的倍数)
 * @param  chr: 要显示的字符(0x20~0x7E)
 * @retval 无
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr);

/**
 * @brief  显示字符串
 * @param  x: 起始X坐标(0~127)
 * @param  y: 起始Y坐标(0~63，8的倍数)
 * @param  str: 要显示的字符串
 * @retval 无
 */
void OLED_ShowString(uint8_t x, uint8_t y, char *str);

/**
 * @brief  刷新GRAM到OLED屏幕
 * @retval 无
 */
void OLED_Refresh(void);

#endif