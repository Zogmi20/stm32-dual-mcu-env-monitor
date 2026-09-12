#ifndef __DELAY_H
#define __DELAY_H

#include "main.h"

// 微秒级延时（简单版：空循环实现）
void delay_us(uint32_t us);

// 毫秒级延时（建议直接用 HAL_Delay，这里为了统一风格）
void delay_ms(uint32_t ms);

#endif
