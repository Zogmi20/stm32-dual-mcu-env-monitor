#ifndef __BSP_KEYPAD_H
#define __BSP_KEYPAD_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

// ==================== 配置宏 ====================
#define KEY_UP_SHORT_PRESS_TIME 50  // 短按判定时间（ms）
#define KEY_UP_TEMP_PRESS_TIME 3000 // 长按3秒 → 温度设置
#define KEY_UP_HUMI_PRESS_TIME 8000 // 长按8秒 → 湿度设置
#define TPAD_TOUCH_THRESHOLD 50     // TPAD触摸阈值

// ==================== 按键事件枚举 ====================
typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_UP_SHORT,
    KEY_EVENT_UP_LONG_TEMP,
    KEY_EVENT_UP_LONG_HUMI,
    KEY_EVENT_TPAD_TOUCH
} KeyEvent_t;

// ==================== 函数声明 ====================
void KeyPad_Init(void);
KeyEvent_t KeyPad_Scan(void);

#endif

