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

// ==================== 私有变量 ====================
static Key_t g_pa0_key = {
    .state = KEY_STATE_IDLE,
    .press_start_time = 0,
    .long_press_triggered = 0,
    .key_pressed = 0};

static SetMode_t g_set_mode = SET_MODE_NORMAL;
static uint16_t g_set_value = 0;

// 保存设置前的原始值 (用于取消)
static float g_old_temp_high;
static float g_old_temp_low;
static float g_old_humi_high;
static float g_old_humi_low;

// PE2/PE4 上次状态 (用于边沿检测)
static uint8_t g_pe2_last_state = 0;
static uint8_t g_pe4_last_state = 0;

// ==================== 私有函数声明 ====================
static void Key_SaveAllSettings(void);
static void Key_ExitWithoutSave(void);
static void Key_Beep(uint16_t duration_ms);
static void Key_EnterNextMode(void);
static void Key_ApplySetValue(uint16_t value);

// ==================== 公有函数实现 ====================

void Key_Init(void)
{
    g_pa0_key.state = KEY_STATE_IDLE;
    g_pa0_key.press_start_time = 0;
    g_pa0_key.long_press_triggered = 0;
    g_pa0_key.key_pressed = 0;
    g_set_mode = SET_MODE_NORMAL;
    g_set_value = 0;
    g_pe2_last_state = 0;
    g_pe4_last_state = 0;

    // 保存当前阈值
    g_old_temp_high = temp_alarm_high;
    g_old_temp_low = temp_alarm_low;
    g_old_humi_high = humi_alarm_high;
    g_old_humi_low = humi_alarm_low;

    HAL_UART_Transmit(&huart1, (uint8_t *)"Key Init OK\r\n", 13, 100);
}

/**
 * @brief  PA0 按键扫描 (非阻塞)
 * @retval 1=有按键事件, 0=无
 */
uint8_t Key_Scan(void)
{
    uint8_t pa0_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);
    uint32_t current_time = HAL_GetTick();

    if (pa0_state == GPIO_PIN_SET)
    {
        switch (g_pa0_key.state)
        {
        case KEY_STATE_IDLE:
            g_pa0_key.state = KEY_STATE_DEBOUNCE;
            g_pa0_key.press_start_time = current_time;
            g_pa0_key.long_press_triggered = 0;
            g_pa0_key.key_pressed = 0;
            break;

        case KEY_STATE_DEBOUNCE:
            if (current_time - g_pa0_key.press_start_time > KEY_DEBOUNCE_TIME)
            {
                g_pa0_key.state = KEY_STATE_PRESSED;
                g_pa0_key.press_start_time = current_time;
                g_pa0_key.key_pressed = 1;
            }
            break;

        case KEY_STATE_PRESSED:
            if (current_time - g_pa0_key.press_start_time > KEY_LONG_PRESS_TIME)
            {
                if (!g_pa0_key.long_press_triggered)
                {
                    g_pa0_key.long_press_triggered = 1;
                    g_pa0_key.key_pressed = 2; // 长按
                    return 1;
                }
            }
            break;

        default:
            break;
        }
        return 0;
    }
    else
    {
        if (g_pa0_key.state == KEY_STATE_PRESSED)
        {
            uint32_t press_duration = current_time - g_pa0_key.press_start_time;
            if (!g_pa0_key.long_press_triggered &&
                press_duration >= KEY_DEBOUNCE_TIME &&
                press_duration < KEY_LONG_PRESS_TIME)
            {
                g_pa0_key.key_pressed = 1; // 短按
                g_pa0_key.state = KEY_STATE_IDLE;
                return 1;
            }
        }

        g_pa0_key.state = KEY_STATE_IDLE;
        g_pa0_key.long_press_triggered = 0;
        g_pa0_key.key_pressed = 0;
        return 0;
    }
}

/**
 * @brief  按键处理主函数 (在 Display 任务中循环调用)
 */
void Key_Process(void)
{
    uint8_t pa0_event = Key_Scan();
    uint8_t pe2_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);
    uint8_t pe4_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4);

    // ============================================================
    // ===== 1. 处理 PE2 (加键) - 下降沿触发 =====
    // ============================================================
    if (pe2_state == GPIO_PIN_RESET && g_pe2_last_state == GPIO_PIN_SET)
    {
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
            {
                g_set_value += 1;
            }
            Key_ApplySetValue(g_set_value);
            Key_Beep(30);
        }
    }
    g_pe2_last_state = pe2_state;

    // ============================================================
    // ===== 2. 处理 PE4 (减键) - 下降沿触发 =====
    // ============================================================
    if (pe4_state == GPIO_PIN_RESET && g_pe4_last_state == GPIO_PIN_SET)
    {
        if (g_set_mode != SET_MODE_NORMAL)
        {
            if (g_set_value > 0)
            {
                g_set_value -= 1;
            }
            Key_ApplySetValue(g_set_value);
            Key_Beep(30);
        }
    }
    g_pe4_last_state = pe4_state;

    // ============================================================
    // ===== 3. 处理 PA0 (设置键) =====
    // ============================================================
    if (pa0_event)
    {
        if (g_pa0_key.key_pressed == 2) // 长按 (3秒)
        {
            if (g_set_mode == SET_MODE_NORMAL)
            {
                // ===== 进入设置模式 =====
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
                // ===== 取消并退出设置 =====
                Key_ExitWithoutSave();
                Key_Beep(100);
                HAL_Delay(100);
                Key_Beep(100);
                HAL_UART_Transmit(&huart1, (uint8_t *)"Cancel\r\n", 8, 100);
            }
            g_pa0_key.key_pressed = 0;
        }
        else if (g_pa0_key.key_pressed == 1) // 短按
        {
            if (g_set_mode != SET_MODE_NORMAL)
            {
                Key_EnterNextMode();
            }
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

/**
 * @brief  取消并退出设置 (不保存)
 */
static void Key_ExitWithoutSave(void)
{
    temp_alarm_low = g_old_temp_low;
    temp_alarm_high = g_old_temp_high;
    humi_alarm_low = g_old_humi_low;
    humi_alarm_high = g_old_humi_high;
    g_set_mode = SET_MODE_NORMAL;
    g_set_value = 0;
}

/**
 * @brief  蜂鸣器响指定毫秒
 */
static void Key_Beep(uint16_t duration_ms)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    osDelay(duration_ms);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
}
