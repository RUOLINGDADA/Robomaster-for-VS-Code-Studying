#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

// ==================== STM32F4 标准 Flash 地址映射 ====================
// 适用于 STM32F407/427/429 等常见型号
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Sector 0, 16 KB */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Sector 1, 16 KB */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Sector 2, 16 KB */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Sector 3, 16 KB */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Sector 4, 64 KB */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Sector 5, 128 KB */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Sector 6, 128 KB */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Sector 7, 128 KB */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Sector 8, 128 KB */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Sector 9, 128 KB */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Sector 10, 128 KB */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Sector 11, 128 KB */
#define FLASH_END_ADDR           ((uint32_t)0x080FFFFF) /* Flash 结束地址 */

// ==================== 测试用用户数据区地址 ====================
// 建议使用 Sector 11 (最后一个扇区) 存储用户参数，避免覆盖程序
#define USER_DATA_FLASH_ADDR     ADDR_FLASH_SECTOR_11
#define USER_DATA_MAX_WORDS      (128 * 1024 / 4) // 128KB = 32768 个 Word

// ==================== 函数声明 ====================

/**
  * @brief  擦除指定 Flash 扇区
  * @param  address: 起始 Flash 地址
  * @param  len: 要擦除的扇区数量
  * @retval 无
  */
void flash_erase_address(uint32_t address, uint16_t len);

/**
  * @brief  向指定 Flash 地址写入数据（单段连续写入）
  * @param  start_address: 起始 Flash 地址
  * @param  buf: 数据指针（Word 对齐）
  * @param  len: 数据长度（单位：Word，4字节）
  * @retval 成功返回 0，失败返回 -1
  */
int8_t flash_write_single_address(uint32_t start_address, uint32_t *buf, uint32_t len);

/**
  * @brief  向指定 Flash 地址范围写入数据
  * @param  start_address: 起始 Flash 地址
  * @param  end_address: 结束 Flash 地址
  * @param  buf: 数据指针（Word 对齐）
  * @param  len: 数据长度（单位：Word，4字节）
  * @retval 成功返回 0，失败返回 -1
  */
int8_t flash_write_multi_address(uint32_t start_address, uint32_t end_address, uint32_t *buf, uint32_t len);

/**
  * @brief  从指定 Flash 地址读取数据
  * @param  address: 起始 Flash 地址
  * @param  buf: 数据存储指针（Word 对齐）
  * @param  len: 数据长度（单位：Word，4字节）
  * @retval 无
  */
void flash_read(uint32_t address, uint32_t *buf, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_FLASH_H */