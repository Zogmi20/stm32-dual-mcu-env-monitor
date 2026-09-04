#include "alarm.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

// ==================== 私有变量 ====================
static Alarm_t g_alarm = {
    .state = ALARM_STATE_NORMAL,
    .type = ALARM_TYPE_NONE,
    .continuous_count = 0,
    .normal_count = 0,
    .triggered = 0,
    .current_temp = 0.0f,
    .current_humi = 0.0f,
    .temp_high_overflow = 0,
    .temp_low_overflow = 0,
    .humi_high_overflow = 0,
    .humi_low_overflow = 0};

// ==================== 私有函数声明 ====================
static void Alarm_Trigger(void);
static void Alarm_Clear(void);
static AlarmType_t Alarm_GetOverflowType(void);

// ==================== 公有函数实现 ====================

void Alarm_Init(void)
{
    g_alarm.state = ALARM_STATE_NORMAL;
    g_alarm.type = ALARM_TYPE_NONE;
    g_alarm.continuous_count = 0;
    g_alarm.normal_count = 0;
    g_alarm.triggered = 0;
    g_alarm.current_temp = 0.0f;
    g_alarm.current_humi = 0.0f;
    g_alarm.temp_high_overflow = 0;
    g_alarm.temp_low_overflow = 0;
    g_alarm.humi_high_overflow = 0;
    g_alarm.humi_low_overflow = 0;

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    HAL_UART_Transmit(&huart1, (uint8_t *)"Alarm Init OK\r\n", 15, 100);
}

void Alarm_Update(float temp, float humi,
                  float temp_high, float temp_low,
                  float humi_high, float humi_low)
{
    uint8_t is_overflow = 0;

    g_alarm.current_temp = temp;
    g_alarm.current_humi = humi;

    // 分别判断四个方向的超标
    g_alarm.temp_high_overflow = (temp >= temp_high) ? 1 : 0;
    g_alarm.temp_low_overflow = (temp <= temp_low) ? 1 : 0;
    g_alarm.humi_high_overflow = (humi >= humi_high) ? 1 : 0;
    g_alarm.humi_low_overflow = (humi <= humi_low) ? 1 : 0;

    // 任意一个超标即视为超标
    if (g_alarm.temp_high_overflow || g_alarm.temp_low_overflow ||
        g_alarm.humi_high_overflow || g_alarm.humi_low_overflow)
    {
        is_overflow = 1;
    }

    if (is_overflow)
    {
        g_alarm.continuous_count++;
        g_alarm.normal_count = 0;

        if (g_alarm.continuous_count > 255)
        {
            g_alarm.continuous_count = 255;
        }

        // 记录报警类型
        g_alarm.type = Alarm_GetOverflowType();

        if (g_alarm.continuous_count >= ALARM_CONTINUOUS_THRESHOLD)
        {
            if (!g_alarm.triggered)
            {
                Alarm_Trigger();
            }
            g_alarm.state = ALARM_STATE_TRIGGERED;
        }
        else
        {
            g_alarm.state = ALARM_STATE_WARNING;
        }
    }
    else
    {
        g_alarm.normal_count++;
        g_alarm.continuous_count = 0;
        g_alarm.type = ALARM_TYPE_NONE;

        if (g_alarm.normal_count > 255)
        {
            g_alarm.normal_count = 255;
        }

        if (g_alarm.triggered)
        {
            if (g_alarm.normal_count >= ALARM_CLEAR_THRESHOLD)
            {
                Alarm_Clear();
                g_alarm.state = ALARM_STATE_NORMAL;
            }
            else
            {
                g_alarm.state = ALARM_STATE_CLEARING;
            }
        }
        else
        {
            g_alarm.state = ALARM_STATE_NORMAL;
        }
    }
}

uint8_t Alarm_IsTriggered(void)
{
    return g_alarm.triggered;
}

AlarmState_t Alarm_GetState(void)
{
    return g_alarm.state;
}

AlarmType_t Alarm_GetType(void)
{
    return g_alarm.type;
}

uint8_t Alarm_GetContinuousCount(void)
{
    return g_alarm.continuous_count;
}

uint8_t Alarm_GetNormalCount(void)
{
    return g_alarm.normal_count;
}

void Alarm_Reset(void)
{
    g_alarm.state = ALARM_STATE_NORMAL;
    g_alarm.type = ALARM_TYPE_NONE;
    g_alarm.continuous_count = 0;
    g_alarm.normal_count = 0;
    g_alarm.triggered = 0;
    g_alarm.temp_high_overflow = 0;
    g_alarm.temp_low_overflow = 0;
    g_alarm.humi_high_overflow = 0;
    g_alarm.humi_low_overflow = 0;

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    HAL_UART_Transmit(&huart1, (uint8_t *)"Alarm Reset\r\n", 13, 100);
}

void Alarm_ManualTrigger(void)
{
    if (!g_alarm.triggered)
    {
        g_alarm.type = ALARM_TYPE_TEMP_HIGH;
        Alarm_Trigger();
        g_alarm.state = ALARM_STATE_TRIGGERED;
        g_alarm.continuous_count = ALARM_CONTINUOUS_THRESHOLD;
        HAL_UART_Transmit(&huart1, (uint8_t *)"Manual Trigger\r\n", 16, 100);
    }
}

void Alarm_ManualClear(void)
{
    if (g_alarm.triggered)
    {
        Alarm_Clear();
        g_alarm.state = ALARM_STATE_NORMAL;
        g_alarm.type = ALARM_TYPE_NONE;
        g_alarm.normal_count = 0;
        HAL_UART_Transmit(&huart1, (uint8_t *)"Manual Clear\r\n", 14, 100);
    }
}

// ==================== 私有函数实现 ====================

static AlarmType_t Alarm_GetOverflowType(void)
{
    uint8_t count = 0;

    if (g_alarm.temp_high_overflow)
        count++;
    if (g_alarm.temp_low_overflow)
        count++;
    if (g_alarm.humi_high_overflow)
        count++;
    if (g_alarm.humi_low_overflow)
        count++;

    if (count >= 2)
        return ALARM_TYPE_BOTH;

    if (g_alarm.temp_high_overflow)
        return ALARM_TYPE_TEMP_HIGH;
    if (g_alarm.temp_low_overflow)
        return ALARM_TYPE_TEMP_LOW;
    if (g_alarm.humi_high_overflow)
        return ALARM_TYPE_HUMI_HIGH;
    if (g_alarm.humi_low_overflow)
        return ALARM_TYPE_HUMI_LOW;

    return ALARM_TYPE_NONE;
}

static void Alarm_Trigger(void)
{
    g_alarm.triggered = 1;

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);

    char buf[128];
    const char *type_str[] = {"", "TEMP HIGH", "TEMP LOW", "HUMI HIGH", "HUMI LOW", "BOTH"};

    sprintf(buf, "!!! ALARM TRIGGERED !!! [%s] Temp:%.1fC, Humi:%.1f%% (Exceeded %d times)\r\n",
            type_str[g_alarm.type],
            g_alarm.current_temp,
            g_alarm.current_humi,
            ALARM_CONTINUOUS_THRESHOLD);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

static void Alarm_Clear(void)
{
    g_alarm.triggered = 0;
    g_alarm.normal_count = 0;
    g_alarm.type = ALARM_TYPE_NONE;

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    char buf[64];
    sprintf(buf, "!!! ALARM CLEARED !!! (Normal %d times)\r\n", ALARM_CLEAR_THRESHOLD);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}
