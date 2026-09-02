#include "alarm.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

// ==================== 私有变量 ====================
static Alarm_t g_alarm = {
    .state = ALARM_STATE_NORMAL,
    .continuous_count = 0,
    .normal_count = 0,
    .triggered = 0,
    .current_temp = 0.0f,
    .current_humi = 0.0f,
    .temp_overflow = 0,
    .humi_overflow = 0};

// ==================== 私有函数声明 ====================
static void Alarm_Trigger(void);
static void Alarm_Clear(void);
static void Alarm_SendDebugInfo(void);

// ==================== 公有函数实现 ====================

/**
 * @brief  报警模块初始化
 */
void Alarm_Init(void)
{
    g_alarm.state = ALARM_STATE_NORMAL;
    g_alarm.continuous_count = 0;
    g_alarm.normal_count = 0;
    g_alarm.triggered = 0;
    g_alarm.current_temp = 0.0f;
    g_alarm.current_humi = 0.0f;
    g_alarm.temp_overflow = 0;
    g_alarm.humi_overflow = 0;

    // 确保蜂鸣器关闭 (PF8)
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    // 确保报警灯关闭 (PF9)
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    HAL_UART_Transmit(&huart1, (uint8_t *)"Alarm Init OK\r\n", 15, 100);
}

/**
 * @brief  更新报警状态
 * @param  temp           当前温度
 * @param  humi           当前湿度
 * @param  temp_threshold 温度报警阈值
 * @param  humi_threshold 湿度报警阈值
 * @note   连续5次超标才触发报警，连续3次正常才清除报警
 */
void Alarm_Update(float temp, float humi, float temp_threshold, float humi_threshold)
{
    uint8_t is_overflow = 0;

    // 保存当前值
    g_alarm.current_temp = temp;
    g_alarm.current_humi = humi;

    // 分别记录温度和湿度是否超标（用于调试）
    g_alarm.temp_overflow = (temp >= temp_threshold) ? 1 : 0;
    g_alarm.humi_overflow = (humi >= humi_threshold) ? 1 : 0;

    // 判断是否超标（温度或湿度任一超标即视为超标）
    if (temp >= temp_threshold || humi >= humi_threshold)
    {
        is_overflow = 1;
    }

    if (is_overflow)
    {
        // ===== 超标：增加超标计数，清零正常计数 =====
        g_alarm.continuous_count++;
        g_alarm.normal_count = 0;

        // 限制最大值，防止溢出
        if (g_alarm.continuous_count > 255)
        {
            g_alarm.continuous_count = 255;
        }

        // 检查是否达到触发阈值（连续5次超标）
        if (g_alarm.continuous_count >= ALARM_CONTINUOUS_THRESHOLD)
        {
            // 触发报警（如果还未触发）
            if (!g_alarm.triggered)
            {
                Alarm_Trigger();
            }
            g_alarm.state = ALARM_STATE_TRIGGERED;
        }
        else
        {
            // 预警状态（超标但次数不够）
            g_alarm.state = ALARM_STATE_WARNING;
        }
    }
    else
    {
        // ===== 正常：增加正常计数，清零超标计数 =====
        g_alarm.normal_count++;
        g_alarm.continuous_count = 0;

        // 限制最大值，防止溢出
        if (g_alarm.normal_count > 255)
        {
            g_alarm.normal_count = 255;
        }

        // 如果报警已触发，检查是否可以清除
        if (g_alarm.triggered)
        {
            // 需要连续正常3次才清除报警
            if (g_alarm.normal_count >= ALARM_CLEAR_THRESHOLD)
            {
                // 清除报警
                Alarm_Clear();
                g_alarm.state = ALARM_STATE_NORMAL;
            }
            else
            {
                // 正在清除中（正常但次数不够）
                g_alarm.state = ALARM_STATE_CLEARING;
            }
        }
        else
        {
            // 正常状态（未触发报警）
            g_alarm.state = ALARM_STATE_NORMAL;
        }
    }

    // 串口调试输出（可选，默认注释）
    // Alarm_SendDebugInfo();
}

/**
 * @brief  获取报警是否已触发
 * @retval 1=已触发, 0=未触发
 */
uint8_t Alarm_IsTriggered(void)
{
    return g_alarm.triggered;
}

/**
 * @brief  获取当前报警状态
 * @retval AlarmState_t 报警状态
 */
AlarmState_t Alarm_GetState(void)
{
    return g_alarm.state;
}

/**
 * @brief  获取报警连续超标计数
 * @retval 连续超标次数 (0-5)
 */
uint8_t Alarm_GetContinuousCount(void)
{
    return g_alarm.continuous_count;
}

/**
 * @brief  获取报警连续正常计数
 * @retval 连续正常次数 (0-3)
 */
uint8_t Alarm_GetNormalCount(void)
{
    return g_alarm.normal_count;
}

/**
 * @brief  复位报警模块
 */
void Alarm_Reset(void)
{
    g_alarm.state = ALARM_STATE_NORMAL;
    g_alarm.continuous_count = 0;
    g_alarm.normal_count = 0;
    g_alarm.triggered = 0;
    g_alarm.temp_overflow = 0;
    g_alarm.humi_overflow = 0;

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    HAL_UART_Transmit(&huart1, (uint8_t *)"Alarm Reset\r\n", 13, 100);
}

/**
 * @brief  手动触发报警（用于按键测试）
 */
void Alarm_ManualTrigger(void)
{
    if (!g_alarm.triggered)
    {
        Alarm_Trigger();
        g_alarm.state = ALARM_STATE_TRIGGERED;
        g_alarm.continuous_count = ALARM_CONTINUOUS_THRESHOLD;
        HAL_UART_Transmit(&huart1, (uint8_t *)"Manual Trigger\r\n", 16, 100);
    }
}

/**
 * @brief  手动清除报警（用于按键测试）
 */
void Alarm_ManualClear(void)
{
    if (g_alarm.triggered)
    {
        Alarm_Clear();
        g_alarm.state = ALARM_STATE_NORMAL;
        g_alarm.normal_count = 0;
        HAL_UART_Transmit(&huart1, (uint8_t *)"Manual Clear\r\n", 14, 100);
    }
}

// ==================== 私有函数实现 ====================

/**
 * @brief  触发报警（内部调用）
 */
static void Alarm_Trigger(void)
{
    g_alarm.triggered = 1;

    // 打开蜂鸣器 (PF8)
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    // 打开报警灯 (PF9) - 假设高电平点亮
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);

    // 串口输出报警信息（包含具体数值）
    char buf[128];
    sprintf(buf, "!!! ALARM TRIGGERED !!! Temp:%.1fC, Humi:%.1f%%(Exceeded %d times continuously)\r\n",
            g_alarm.current_temp,
            g_alarm.current_humi,
            ALARM_CONTINUOUS_THRESHOLD);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

/**
 * @brief  清除报警（内部调用）
 */
static void Alarm_Clear(void)
{
    g_alarm.triggered = 0;
    g_alarm.normal_count = 0;

    // 关闭蜂鸣器 (PF8)
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
    // 关闭报警灯 (PF9)
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);

    // 串口输出清除信息
    char buf[64];
    sprintf(buf, "!!! ALARM CLEARED !!!(Normal for %d times)\r\n", ALARM_CLEAR_THRESHOLD);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

/**
 * @brief  串口调试输出（内部调用）
 */
static void Alarm_SendDebugInfo(void)
{
    char buf[128];
    const char *state_str[] = {"NORMAL", "WARNING", "TRIGGERED", "CLEARING"};
    sprintf(buf, "[Alarm] State:%s, Cont:%d/%d, Norm:%d/%d, Trig:%d, T:%d H:%d\r\n",
            state_str[g_alarm.state],
            g_alarm.continuous_count,
            ALARM_CONTINUOUS_THRESHOLD,
            g_alarm.normal_count,
            ALARM_CLEAR_THRESHOLD,
            g_alarm.triggered,
            g_alarm.temp_overflow,
            g_alarm.humi_overflow);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}
