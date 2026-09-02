#ifndef __ALARM_H
#define __ALARM_H

#include "main.h"
#include "stdint.h"

// ==================== 宏定义 ====================
#define ALARM_CONTINUOUS_THRESHOLD 5 // 连续超标5次才触发报警
#define ALARM_CLEAR_THRESHOLD 3      // 连续正常3次才清除报警

// ==================== 枚举定义 ====================
typedef enum
{
    ALARM_STATE_NORMAL = 0, // 正常状态
    ALARM_STATE_WARNING,    // 预警状态（超标但未达触发次数）
    ALARM_STATE_TRIGGERED,  // 报警已触发
    ALARM_STATE_CLEARING    // 正在清除中（正常但未达清除次数）
} AlarmState_t;

// ==================== 结构体定义 ====================
typedef struct
{
    AlarmState_t state;       // 当前状态
    uint8_t continuous_count; // 连续超标计数
    uint8_t normal_count;     // 连续正常计数
    uint8_t triggered;        // 报警触发标志 (1=触发, 0=未触发)
    float current_temp;       // 当前温度（用于调试）
    float current_humi;       // 当前湿度（用于调试）
    uint8_t temp_overflow;    // 温度是否超标
    uint8_t humi_overflow;    // 湿度是否超标
} Alarm_t;

// ==================== 函数声明 ====================

/**
 * @brief  报警模块初始化
 */
void Alarm_Init(void);

/**
 * @brief  更新报警状态
 * @param  temp           当前温度
 * @param  humi           当前湿度
 * @param  temp_threshold 温度报警阈值
 * @param  humi_threshold 湿度报警阈值
 * @note   每次收到新传感器数据时调用
 * @note   连续5次超标才触发报警，连续3次正常才清除报警
 */
void Alarm_Update(float temp, float humi, float temp_threshold, float humi_threshold);

/**
 * @brief  获取报警是否已触发
 * @retval 1=已触发, 0=未触发
 */
uint8_t Alarm_IsTriggered(void);

/**
 * @brief  获取当前报警状态
 * @retval AlarmState_t 报警状态
 */
AlarmState_t Alarm_GetState(void);

/**
 * @brief  获取报警连续超标计数
 * @retval 连续超标次数 (0-5)
 */
uint8_t Alarm_GetContinuousCount(void);

/**
 * @brief  获取报警连续正常计数
 * @retval 连续正常次数 (0-3)
 */
uint8_t Alarm_GetNormalCount(void);

/**
 * @brief  复位报警模块
 */
void Alarm_Reset(void);

/**
 * @brief  手动触发报警（用于按键测试）
 */
void Alarm_ManualTrigger(void);

/**
 * @brief  手动清除报警（用于按键测试）
 */
void Alarm_ManualClear(void);

#endif /* __ALARM_H */

