#include "keypad_input.h"
#include "alarm.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

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

// PE2 上次状态 (用于边沿检测)
static uint8_t g_pe2_last_state = 0;

// ==================== 私有函数声明 ====================
static void Key_SaveAllSettings(void);
static void Key_ExitWithoutSave(void);
static void Key_Beep(uint16_t duration_ms);
static void Key_EnterNextMode(void);

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

    // 保存当前阈值
    g_old_temp_high = temp_alarm_high;
    g_old_temp_low = temp_alarm_low;
    g_old_humi_high = humi_alarm_high;
    g_old_humi_low = humi_alarm_low;

    HAL_UART_Transmit(&huart1, (uint8_t *)"Key Init OK\r\n", 13, 100);
}

/**
 * @brief  按键扫描 (非阻塞)
 * @retval 1=PA0被按下, 0=无按键
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
            // 检测长按 (3秒)
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
        // 按键释放
        if (g_pa0_key.state == KEY_STATE_PRESSED)
        {
            uint32_t press_duration = current_time - g_pa0_key.press_start_time;
            // 短按: 按下时间 > 消抖时间 且 < 长按时间
            if (!g_pa0_key.long_press_triggered &&
                press_duration >= KEY_DEBOUNCE_TIME &&
                press_duration < KEY_LONG_PRESS_TIME)
            {
                g_pa0_key.key_pressed = 1;
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
 * @brief  处理按键事件
 */
void Key_Process(void)
{
    uint8_t pa0_pressed = Key_Scan();
    uint8_t pe2_state = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_2);

    // ===== 处理 PE2 (ADD键) - 边沿检测 =====
    if (pe2_state == GPIO_PIN_SET && g_pe2_last_state == GPIO_PIN_RESET)
    {
        // PE2 上升沿触发
        if (g_set_mode != SET_MODE_NORMAL)
        {
            // 在设置模式下，PE2 用于增加值
            switch (g_set_mode)
            {
            case SET_MODE_TEMP_LOW:
            case SET_MODE_TEMP_HIGH:
                if (g_set_value < 99)
                    g_set_value += 1;
                Key_Beep(30);
                break;

            case SET_MODE_HUMI_LOW:
            case SET_MODE_HUMI_HIGH:
                if (g_set_value < 100)
                    g_set_value += 5;
                if (g_set_value > 100)
                    g_set_value = 100;
                Key_Beep(30);
                break;

            default:
                break;
            }

            char buf[32];
            sprintf(buf, "Set: %d\r\n", g_set_value);
            HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
        }
    }
    g_pe2_last_state = pe2_state;

    // ===== 处理 PA0 (SET键) =====
    if (pa0_pressed)
    {
        if (g_pa0_key.key_pressed == 2) // 长按 (3秒)
        {
            if (g_set_mode == SET_MODE_NORMAL)
            {
                // ===== 从正常模式进入设置模式 =====
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
                // ===== 在设置模式下长按 → 取消并退出 =====
                Key_ExitWithoutSave();
                Key_Beep(100);
                HAL_Delay(100);
                Key_Beep(100); // 两声表示取消

                HAL_UART_Transmit(&huart1, (uint8_t *)"Exit Set Mode (Cancel)\r\n", 25, 100);
            }
        }
        else if (g_pa0_key.key_pressed == 1) // 短按 (确认)
        {
            if (g_set_mode != SET_MODE_NORMAL)
            {
                // 进入下一步 或 保存
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
 * @brief  进入下一个设置模式
 */
static void Key_EnterNextMode(void)
{
    // 保存当前设置的值
    switch (g_set_mode)
    {
    case SET_MODE_TEMP_LOW:
        temp_alarm_low = (float)g_set_value;
        g_set_mode = SET_MODE_TEMP_HIGH;
        g_set_value = (uint16_t)temp_alarm_high;
        Key_Beep(80);
        break;

    case SET_MODE_TEMP_HIGH:
        temp_alarm_high = (float)g_set_value;
        g_set_mode = SET_MODE_HUMI_LOW;
        g_set_value = (uint16_t)humi_alarm_low;
        Key_Beep(80);
        break;

    case SET_MODE_HUMI_LOW:
        humi_alarm_low = (float)g_set_value;
        g_set_mode = SET_MODE_HUMI_HIGH;
        g_set_value = (uint16_t)humi_alarm_high;
        Key_Beep(80);
        break;

    case SET_MODE_HUMI_HIGH:
        humi_alarm_high = (float)g_set_value;
        // 保存完成
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

    g_set_mode = SET_MODE_NORMAL;
    Key_Beep(800); // 长响表示保存成功

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
    // 恢复旧值
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
    HAL_Delay(duration_ms);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
}
