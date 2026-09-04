#ifndef __APP_TYPES_H
#define __APP_TYPES_H

#include "main.h"

// ===== 传感器消息结构（所有任务共享） =====
typedef struct
{
    float temp;
    float humi;
    uint8_t ok; // 1=成功，0=失败
} SensorMsg_t;

#endif
