#include "keytpad_input.h"
#include "alarm.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include "flash.h"
#include "cmsis_os.h"

// ==================== 外部变量 ====================
extern float temp_alarm_high;
extern float temp_alarm_low;
extern float humi_alarm_high;
extern float humi_alarm_low;

#define PE_DEBOUNCE_MS 20

static SetMode_t g_set_mode = SET_MODE_NORMAL;
static uint16_t g_set_value = 0;

// 保存设置前的原始值 (用于取消)
static float g_old_temp_high;
static float g_old_temp_low;
static float g_old_humi_high;
static float g_old_humi_low;
static uint8_t g_pa0_key_up = 0;
static volatile uint8_t g_pa0_irq_flag = 0;
static volatile uint32_t g_pa0_irq_tick = 0;
// PE2/PE4 上次状态 (用于边沿检测)
static uint8_t g_pe2_last_state = 0;
static uint8_t g_pe4_last_state = 0;
// ==================== 私有函数声明 ====================
static void Key_SaveAllSettings(void);
static void Key_Beep(uint16_t duration_ms);
static void Key_EnterNextMode(void);
static void Key_ApplySetValue(uint16_t value);
    // ==================== 公有函数实现 ====================
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0)
    {
        g_pa0_irq_flag = 1;
        g_pa0_irq_tick = HAL_GetTick();
    }
}
void Key_Init(void)
{
  
    g_set_mode = SET_MODE_NORMAL;
    g_set_value = 0;
    g_pe2_last_state = 0;
    g_pe4_last_state = 0;

    // 保存当前阈值
    g_old_temp_high = temp_alarm_high;
    g_old_temp_low = temp_alarm_low;
    g_old_humi_high = humi_alarm_high;
    g_old_humi_low = humi_alarm_low;

   
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET); // 确保蜂鸣器关
    HAL_UART_Transmit(&huart1, (uint8_t *)"Key Init OK\r\n", 13, 100);
}

/**
 * @brief  按键处理主函数 (在 Display 任务中循环调用)
 */
void Key_Process(void)
{
    uint32_t current_time = HAL_GetTick();

    // ===== PA0 中断标志消费 =====
    uint8_t pa0_event = 0;
    static uint32_t last_pa0_tick = 0;

    if (g_pa0_irq_flag)
    {
        g_pa0_irq_flag = 0;

        if (current_time - last_pa0_tick > 20) // 20ms 消抖
        {
            last_pa0_tick = current_time;
            g_pa0_key_up = 0; // ★ 必须松手才能再触发
            pa0_event = 1;
        }
    }
    // ★ 松手复位
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
        g_pa0_key_up = 1;
    }
    uint8_t pe2_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);
    uint8_t pe4_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);

    static uint32_t pe2_tick = 0;
    static uint32_t pe4_tick = 0;

    // ============================================================
    // ===== 1. PE2 加键（下降沿 + 20ms 消抖）=====
    // ============================================================
    if (pe2_state == GPIO_PIN_RESET && g_pe2_last_state == GPIO_PIN_SET && (current_time - pe2_tick > PE_DEBOUNCE_MS))
    {
        pe2_tick = current_time;
        if (g_set_mode != SET_MODE_NORMAL)
        {
            uint16_t max_val = 0;
            switch (g_set_mode)
            {
            case SET_MODE_TEMP_LOW:
            case SET_MODE_TEMP_HIGH:
                max_val = 99;
                break;
            case SET_MODE_HUMI_LOW:
            case SET_MODE_HUMI_HIGH:
                max_val = 100;
                break;
            default:
                break;
            }
            if (g_set_value < max_val)
                g_set_value += 1;
            Key_ApplySetValue(g_set_value);
            Key_Beep(30);
        }
    }
    g_pe2_last_state = pe2_state;

    // ============================================================
    // ===== 2. PE4 减键（下降沿 + 20ms 消抖）=====
    // ============================================================
    if (pe4_state == GPIO_PIN_RESET && g_pe4_last_state == GPIO_PIN_SET && (current_time - pe4_tick > PE_DEBOUNCE_MS))
    {
        pe4_tick = current_time;
        if (g_set_mode != SET_MODE_NORMAL)
        {
            if (g_set_value > 0)
                g_set_value -= 1;
            Key_ApplySetValue(g_set_value);
            Key_Beep(30);
        }
    }
    g_pe4_last_state = pe4_state;

    // ============================================================
    // ===== 3. PA0 设置键 =====
    // ============================================================
    if (pa0_event == 1)
    {
        if (g_set_mode == SET_MODE_NORMAL)
        {
            // 进入设置模式
            g_set_mode = SET_MODE_TEMP_LOW;
            g_set_value = (uint16_t)temp_alarm_low;
            g_old_temp_low = temp_alarm_low;
            g_old_temp_high = temp_alarm_high;
            g_old_humi_low = humi_alarm_low;
            g_old_humi_high = humi_alarm_high;
            Key_Beep(150);
            HAL_UART_Transmit(&huart1, (uint8_t *)"Enter Set Mode\r\n", 16, 100);
        }
        else
        {
            Key_EnterNextMode();
        }
    }
}

SetMode_t Key_GetSetMode(void)
{
    return g_set_mode;
}

uint16_t Key_GetSetValue(void)
{
    return g_set_value;
}

// ==================== 私有函数实现 ====================

/**
 * @brief  应用当前设置值到对应的阈值
 */
static void Key_ApplySetValue(uint16_t value)
{
    switch (g_set_mode)
    {
    case SET_MODE_TEMP_LOW:
        temp_alarm_low = (float)value;
        break;
    case SET_MODE_TEMP_HIGH:
        temp_alarm_high = (float)value;
        break;
    case SET_MODE_HUMI_LOW:
        humi_alarm_low = (float)value;
        break;
    case SET_MODE_HUMI_HIGH:
        humi_alarm_high = (float)value;
        break;
    default:
        break;
    }

    char buf[32];
    sprintf(buf, "Set: %d\r\n", value);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

/**
 * @brief  进入下一个设置模式
 */
static void Key_EnterNextMode(void)
{
    // 先应用当前值
    Key_ApplySetValue(g_set_value);

    switch (g_set_mode)
    {
    case SET_MODE_TEMP_LOW:
        g_set_mode = SET_MODE_TEMP_HIGH;
        g_set_value = (uint16_t)temp_alarm_high;
        Key_Beep(80);
        break;

    case SET_MODE_TEMP_HIGH:
        g_set_mode = SET_MODE_HUMI_LOW;
        g_set_value = (uint16_t)humi_alarm_low;
        Key_Beep(80);
        break;

    case SET_MODE_HUMI_LOW:
        g_set_mode = SET_MODE_HUMI_HIGH;
        g_set_value = (uint16_t)humi_alarm_high;
        Key_Beep(80);
        break;

    case SET_MODE_HUMI_HIGH:
        humi_alarm_high = (float)g_set_value;
        Key_SaveAllSettings();
        break;

    default:
        break;
    }

    char buf[64];
    sprintf(buf, "Mode:%d, Value:%d\r\n", g_set_mode, g_set_value);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

/**
 * @brief  保存所有设置
 */
static void Key_SaveAllSettings(void)
{
    // 检查温度上下限是否合理
    if (temp_alarm_low >= temp_alarm_high)
    {
        temp_alarm_low = temp_alarm_high - 5.0f;
        if (temp_alarm_low < 0)
            temp_alarm_low = 0;
    }

    if (humi_alarm_low >= humi_alarm_high)
    {
        humi_alarm_low = humi_alarm_high - 20.0f;
        if (humi_alarm_low < 0)
            humi_alarm_low = 0;
    }
    
    uint8_t need_save = 0;

    if (temp_alarm_high != g_old_temp_high ||
        temp_alarm_low != g_old_temp_low ||
        humi_alarm_high != g_old_humi_high ||
        humi_alarm_low != g_old_humi_low)
    {
        need_save = 1;
    }
 
    if (need_save)
    {
        // ===== ✅ 保存所有阈值到 Flash =====
        Flash_Save_All(temp_alarm_high, temp_alarm_low, humi_alarm_high, humi_alarm_low);

        g_set_mode = SET_MODE_NORMAL;
        Key_Beep(800);

        char buf[64];
        sprintf(buf, "Saved! T:%.1f~%.1f, H:%.1f~%.1f\r\n",
                temp_alarm_low, temp_alarm_high,
                humi_alarm_low, humi_alarm_high);
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
    }
    else
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)"No change\r\n", 12, 100);
        Key_Beep(200);
    }
    /* ★ 无论是否保存，都退出设置模式 */
    g_set_mode = SET_MODE_NORMAL;
}

/**
 * @brief  蜂鸣器响指定毫秒
 */
    static void Key_Beep(uint16_t duration_ms)
    {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
        osDelay(duration_ms);                                 // 原地等
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET); // 直接关
    }
