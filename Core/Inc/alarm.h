#ifndef __ALARM_H
#define __ALARM_H

#include "main.h"
#include "stdint.h"

// ==================== 宏定义 ====================
#define ALARM_CONTINUOUS_THRESHOLD 5 // 连续超标5次触发报警
#define ALARM_CLEAR_THRESHOLD 3      // 连续正常3次清除报警

// ==================== 枚举定义 ====================
typedef enum
{
    ALARM_STATE_NORMAL = 0,
    ALARM_STATE_WARNING,
    ALARM_STATE_TRIGGERED,
    ALARM_STATE_CLEARING
} AlarmState_t;

// ==================== 报警类型枚举 ====================
typedef enum
{
    ALARM_TYPE_NONE = 0,
    ALARM_TYPE_TEMP_HIGH, // 温度过高
    ALARM_TYPE_TEMP_LOW,  // 温度过低
    ALARM_TYPE_HUMI_HIGH, // 湿度过高
    ALARM_TYPE_HUMI_LOW,  // 湿度过低
    ALARM_TYPE_BOTH       // 多个同时超标
} AlarmType_t;

// ==================== 结构体定义 ====================
typedef struct
{
    AlarmState_t state;
    AlarmType_t type; // 当前报警类型
    uint8_t continuous_count;
    uint8_t normal_count;
    uint8_t triggered;
    float current_temp;
    float current_humi;
    uint8_t temp_high_overflow;
    uint8_t temp_low_overflow;
    uint8_t humi_high_overflow;
    uint8_t humi_low_overflow;
} Alarm_t;

// ==================== 函数声明 ====================

void Alarm_Init(void);

/**
 * @brief  更新报警状态
 * @param  temp           当前温度
 * @param  humi           当前湿度
 * @param  temp_high      温度上限
 * @param  temp_low       温度下限
 * @param  humi_high      湿度上限
 * @param  humi_low       湿度下限
 */
void Alarm_Update(float temp, float humi,
                  float temp_high, float temp_low,
                  float humi_high, float humi_low);

uint8_t Alarm_IsTriggered(void);
AlarmState_t Alarm_GetState(void);
AlarmType_t Alarm_GetType(void);
uint8_t Alarm_GetContinuousCount(void);
uint8_t Alarm_GetNormalCount(void);
void Alarm_Reset(void);
void Alarm_ManualTrigger(void);
void Alarm_ManualClear(void);

#endif /* __ALARM_H */
