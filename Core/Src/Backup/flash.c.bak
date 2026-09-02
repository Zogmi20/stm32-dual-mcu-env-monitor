#include "flash.h"
#include "stdio.h"
#include "stm32f4xx_hal_flash.h"

/* 内部函数：根据地址计算对应的扇区索引 */
static uint32_t get_sector_index(uint32_t addr)
{
    if (addr < ADDR_FLASH_SECTOR_4) return FLASH_SECTOR_0;
    else if (addr < ADDR_FLASH_SECTOR_5) return FLASH_SECTOR_4;
    else if (addr < ADDR_FLASH_SECTOR_6) return FLASH_SECTOR_5;
    else if (addr < ADDR_FLASH_SECTOR_7) return FLASH_SECTOR_6;
    else if (addr < ADDR_FLASH_SECTOR_8) return FLASH_SECTOR_7;
    else if (addr < ADDR_FLASH_SECTOR_9) return FLASH_SECTOR_8;
    else if (addr < ADDR_FLASH_SECTOR_10) return FLASH_SECTOR_9;
    else if (addr < ADDR_FLASH_SECTOR_11) return FLASH_SECTOR_10;
    else return FLASH_SECTOR_11;
}

/* 1. 读一个32位字 */
uint32_t stmflash_read_word(uint32_t faddr)
{
    return *(volatile uint32_t *)faddr;
}

/* 2. 擦除一个扇区（配合安全保护） */
void stmflash_erase_sector(uint32_t sector_index)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error = 0;

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_init.Sector = sector_index;
    erase_init.NbSectors = 1;
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();
    HAL_FLASHEx_Erase(&erase_init, &sector_error);
    HAL_FLASH_Lock();
}

/* 3. 写入指定长度的数据（带地址对齐和扇区保护） */
void stmflash_write(uint32_t waddr, uint32_t *pbuf, uint32_t length)
{
    uint32_t i, sector_index;
    uint32_t sector_start, sector_size, sector_end;

    /* 【安全检查1】判断地址是否合法（必须在 0x08000000 之后） */
    if (waddr < STM32_FLASH_BASE || waddr >= (STM32_FLASH_BASE + STM32_FLASH_SIZE)) 
    {
        printf("Error: 写入地址非法！\r\n");
        return;
    }

    /* 【安全检查2】地址对齐检查！必须能被4整除！否则写不进去！ */
    if (waddr % 4 != 0)
    {
        printf("Error: 地址未4字节对齐！写入失败！\r\n");
        return;
    }

    /* 计算所在扇区 */
    sector_index = get_sector_index(waddr);
    if (sector_index == FLASH_SECTOR_0 || sector_index == FLASH_SECTOR_1 ||
        sector_index == FLASH_SECTOR_2 || sector_index == FLASH_SECTOR_3)
    {
        /* 【绝对封死！】0~3扇区是程序（Bootloader）区，绝对不能碰！否则变砖！ */
        printf("Error: 程序区扇区(0~3)被禁止写入！\r\n");
        return;
    }

    /* 计算当前扇区的大小 */
    if (sector_index == FLASH_SECTOR_4) sector_size = 0x10000; /* 64K */
    else if (sector_index == FLASH_SECTOR_5) sector_size = 0x20000; /* 128K */
    else sector_size = 0x20000; /* 其他大扇区都是 128K */

    sector_start = waddr; 
    sector_end = sector_start + sector_size;

    /* 【安全检查3】检查写入长度是否跨越了本扇区边界 */
    if (waddr + (length * 4) > sector_end)
    {
        printf("Error: 写入长度跨越了扇区边界！\r\n");
        return;
    }

    /* 开始写入流程：先擦除当前扇区，再解锁，写入，再锁定 */
    stmflash_erase_sector(sector_index);

    HAL_FLASH_Unlock();
    for (i = 0; i < length; i++)
    {
        /* 【核心】每次地址加 4（i*4），确保每个字节都在4字节对齐的位置！ */
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, waddr + i * 4, pbuf[i]);
    }
    HAL_FLASH_Lock();
}

/* 4. 读取指定长度的数据 */
void stmflash_read(uint32_t raddr, uint32_t *pbuf, uint32_t length)
{
    uint32_t i;
    for (i = 0; i < length; i++)
    {
        pbuf[i] = stmflash_read_word(raddr + i * 4);
    }
}

/* 5. 测试写入：记住必须在 main.c 里定义安全的地址！ */
void test_write(uint32_t waddr, uint32_t wdata)
{
    uint32_t data;
    stmflash_write(waddr, &wdata, 1);   /* 写入函数内部会自动判断扇区、对齐、擦除 */
    data = stmflash_read_word(waddr);
    printf("Write: 0x%08X, Read: 0x%08X\r\n", wdata, data);
}





#define FLASH_SAVE_ADDR_TEMP    0x080E0000   //温度存放地址
#define FLASH_SAVE_ADDR_HUMI    0x080E0004   //湿度存放地址

//保存温度阈值
void Flash_Save_TempThreshold(float val)
{
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    uint32_t err;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_11;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&erase,&err);
    
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SAVE_ADDR_TEMP, *(uint32_t*)&val);
    HAL_FLASH_Lock();
}

//读取温度阈值
float Flash_Read_TempThreshold(void)
{
    float val;
    val = *(float*)FLASH_SAVE_ADDR_TEMP;
    if(val < 0 || val > 60)
    {
        val = 28.0f;    //默认温度28℃
    }
    return val;
}

//====新增湿度阈值函数====
void Flash_Save_HumiThreshold(float val)
{
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase;
    uint32_t err;
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_11;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&erase,&err);
    
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SAVE_ADDR_HUMI, *(uint32_t*)&val);
    HAL_FLASH_Lock();
}

float Flash_Read_HumiThreshold(void)
{
    float val;
    val = *(float*)FLASH_SAVE_ADDR_HUMI;
    if(val < 0 || val > 100)
    {
        val = 75.0f;    //默认湿度75%
    }
    return val;
}

