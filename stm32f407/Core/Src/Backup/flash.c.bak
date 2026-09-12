#include "flash.h"
#include "stdio.h"
#include <math.h>
#include "stm32f4xx_hal_flash.h"

// ===== 扇区 11 存储地址（4 个 float = 16 字节） =====
#define FLASH_SAVE_ADDR_TEMP_HIGH 0x080E0000 // 温度上限
#define FLASH_SAVE_ADDR_TEMP_LOW 0x080E0004  // 温度下限
#define FLASH_SAVE_ADDR_HUMI_HIGH 0x080E0008 // 湿度上限
#define FLASH_SAVE_ADDR_HUMI_LOW 0x080E000C  // 湿度下限

// ===== 默认值 =====
#define DEFAULT_TEMP_HIGH 35.0f
#define DEFAULT_TEMP_LOW 5.0f
#define DEFAULT_HUMI_HIGH 95.0f
#define DEFAULT_HUMI_LOW 30.0f

// ===== 擦除扇区 11 =====
static void Flash_EraseSector11(void)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t err;

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_11;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    __disable_irq();
    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&erase, &err);
    HAL_FLASH_Lock();
    __enable_irq();
}
// ===== 写入一个 float 到指定地址（不擦除，由调用者控制擦除） =====
static void Flash_WriteFloat(uint32_t addr, float val)
{
    __disable_irq(); // ← 关闭所有中断
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *(uint32_t *)&val);
    HAL_FLASH_Lock();
    __enable_irq(); // ← 恢复中断
}

// ===== 保存温度上限 =====
void Flash_Save_TempHigh(float val)
{
    float old_val = Flash_Read_TempHigh();
    if (old_val == val)
        return; // 值一样，不写入

    Flash_EraseSector11();
    Flash_WriteFloat(FLASH_SAVE_ADDR_TEMP_HIGH, val);
}

// ===== 读取温度上限 =====
float Flash_Read_TempHigh(void)
{
    float val = *(float *)FLASH_SAVE_ADDR_TEMP_HIGH;
    if (val != val || val < 0 || val > 60)
        val = DEFAULT_TEMP_HIGH; // 默认 35°C
    return val;
}

// ===== 保存温度下限 =====
void Flash_Save_TempLow(float val)
{
    float old_val = Flash_Read_TempLow();
    if (old_val == val)
        return; // 值一样，不写入
    Flash_EraseSector11();
    Flash_WriteFloat(FLASH_SAVE_ADDR_TEMP_LOW, val);
}

// ===== 读取温度下限 =====
float Flash_Read_TempLow(void)
{
    float val = *(float *)FLASH_SAVE_ADDR_TEMP_LOW;
    if (val != val || val < 0 || val > 60)
        val = DEFAULT_TEMP_LOW; // 默认 5°C
    return val;
}

// ===== 保存湿度上限 =====
void Flash_Save_HumiHigh(float val)
{
    float old_val = Flash_Read_HumiHigh();
    if (old_val == val)
        return; // 值一样，不写入

    Flash_EraseSector11();
    Flash_WriteFloat(FLASH_SAVE_ADDR_HUMI_HIGH, val);
}

// ===== 读取湿度上限 =====
float Flash_Read_HumiHigh(void)
{
    float val = *(float *)FLASH_SAVE_ADDR_HUMI_HIGH;
    if (val != val || val < 0 || val > 100)
        val = DEFAULT_HUMI_HIGH; // 默认 95%
    return val;
}

// ===== 保存湿度下限 =====
void Flash_Save_HumiLow(float val)
{
    float old_val = Flash_Read_HumiLow();
    if (old_val == val)
        return; // 值一样，不写入

    Flash_EraseSector11();
    Flash_WriteFloat(FLASH_SAVE_ADDR_HUMI_LOW, val);
}

// ===== 读取湿度下限 =====
float Flash_Read_HumiLow(void)
{
    float val = *(float *)FLASH_SAVE_ADDR_HUMI_LOW;
    if (val != val || val < 0 || val > 100)
        val = DEFAULT_HUMI_LOW; // 默认 30%
    return val;
}

// ===== ? 推荐：一次性保存所有阈值（只擦一次，值一样不写） =====
void Flash_Save_All(float temp_high, float temp_low, float humi_high, float humi_low)
{
    // ===== 1?? 参数合法性检查 =====
    if (temp_low >= temp_high)
    {
        temp_low = temp_high - 5.0f;
        if (temp_low < 0)
            temp_low = 0;
    }
    if (humi_low >= humi_high)
    {
        humi_low = humi_high - 20.0f;
        if (humi_low < 0)
            humi_low = 0;
    }
    if (humi_high > 100)
        humi_high = 100;
    if (humi_low < 0)
        humi_low = 0;

    // ===== 2?? 读取当前值 =====
    float old_th = Flash_Read_TempHigh();
    float old_tl = Flash_Read_TempLow();
    float old_hh = Flash_Read_HumiHigh();
    float old_hl = Flash_Read_HumiLow();

    // ===== 3?? ? 用误差范围比较（而不是直接 ==） =====
    if (fabsf(old_th - temp_high) < 0.01f &&
        fabsf(old_tl - temp_low) < 0.01f &&
        fabsf(old_hh - humi_high) < 0.01f &&
        fabsf(old_hl - humi_low) < 0.01f)
    {
        return; // 值没变，不写入
    }

    // ===== 4?? 擦除并写入 =====
    Flash_EraseSector11();
    Flash_WriteFloat(FLASH_SAVE_ADDR_TEMP_HIGH, temp_high);
    Flash_WriteFloat(FLASH_SAVE_ADDR_TEMP_LOW, temp_low);
    Flash_WriteFloat(FLASH_SAVE_ADDR_HUMI_HIGH, humi_high);
    Flash_WriteFloat(FLASH_SAVE_ADDR_HUMI_LOW, humi_low);
}

// ===== 读取所有 4 个阈值 =====
void Flash_Read_All(float *temp_high, float *temp_low, float *humi_high, float *humi_low)
{
    *temp_high = Flash_Read_TempHigh();
    *temp_low = Flash_Read_TempLow();
    *humi_high = Flash_Read_HumiHigh();
    *humi_low = Flash_Read_HumiLow();
}
