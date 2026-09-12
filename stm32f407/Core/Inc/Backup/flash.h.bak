#ifndef __STMFLASH_H
#define __STMFLASH_H

#include "sys.h"

/* FLASH起始地址 */
#define STM32_FLASH_SIZE 0x100000
#define STM32_FLASH_BASE 0x08000000
#define FLASH_WAITETIME 50000

/* FLASH 扇区的起始地址 */
#define ADDR_FLASH_SECTOR_0 ((uint32_t)0x08000000)
#define ADDR_FLASH_SECTOR_1 ((uint32_t)0x08004000)
#define ADDR_FLASH_SECTOR_2 ((uint32_t)0x08008000)
#define ADDR_FLASH_SECTOR_3 ((uint32_t)0x0800C000)
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)
#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)
#define ADDR_FLASH_SECTOR_6 ((uint32_t)0x08040000)
#define ADDR_FLASH_SECTOR_7 ((uint32_t)0x08060000)
#define ADDR_FLASH_SECTOR_8 ((uint32_t)0x08080000)
#define ADDR_FLASH_SECTOR_9 ((uint32_t)0x080A0000)
#define ADDR_FLASH_SECTOR_10 ((uint32_t)0x080C0000)
#define ADDR_FLASH_SECTOR_11 ((uint32_t)0x080E0000)

// ===== 基础 Flash 操作 =====
uint32_t stmflash_read_word(uint32_t faddr);
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length);
void stmflash_read(uint32_t raddr, uint32_t *pbuf, uint32_t length);
void test_write(uint32_t waddr, uint32_t wdata);

// ===== 4 个阈值读写 =====
void Flash_Save_TempHigh(float val);
float Flash_Read_TempHigh(void);
void Flash_Save_TempLow(float val);
float Flash_Read_TempLow(void);
void Flash_Save_HumiHigh(float val);
float Flash_Read_HumiHigh(void);
void Flash_Save_HumiLow(float val);
float Flash_Read_HumiLow(void);

// ===== 批量读写（推荐） =====
void Flash_Save_All(float temp_high, float temp_low, float humi_high, float humi_low);
void Flash_Read_All(float *temp_high, float *temp_low, float *humi_high, float *humi_low);

#endif
