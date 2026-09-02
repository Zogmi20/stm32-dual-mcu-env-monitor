#include "keytpad_input.h"
#include "usart.h"
#include <string.h>

// ==================== 私有变量 ====================
// KEY_UP 状态PA0
static uint8_t key_up_prev = 0;
static uint32_t key_up_press_start = 0;
static uint8_t key_up_event_sent = 0;

// TPAD 状态PA5
static uint16_t tpad_default_val = 0;
static uint8_t tpad_init_done = 0;
static uint8_t tpad_prev = 0;

// ==================== 私有函数声明 ====================
static void TPAD_Init(void);
static uint16_t TPAD_GetValue(void);
static uint8_t TPAD_Scan(void);

// ==================== 蜂鸣器提示函数 ====================

static void BEEP_Short(uint16_t duration_ms)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    osDelay(duration_ms);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void BEEP_EnterTempMode(void)
{
    for (int i = 0; i < 3; i++)
    {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
        osDelay(200);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
        osDelay(200);
    }
}

static void BEEP_EnterHumiMode(void)
{
    for (int i = 0; i < 5; i++)
    {
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
        osDelay(100);
    }
}

static void BEEP_SaveSuccess(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    osDelay(500);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
}

static void BEEP_TPADTouch(void)
{
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);
    osDelay(30);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
}

// ==================== 公有函数实现 ====================

void KeyPad_Init(void)
{
    TPAD_Init();

    char buf[64];
    sprintf(buf, "KeyPad Init OK, TPAD Default: %d\r\n", tpad_default_val);
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
}

KeyEvent_t KeyPad_Scan(void)
{
    KeyEvent_t event = KEY_EVENT_NONE;

    // ===== 1. 扫描 KEY_UP =====
    uint8_t key_up_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

    if (key_up_now == GPIO_PIN_SET)
    {
        if (!key_up_prev)
        {
            key_up_press_start = xTaskGetTickCount();
            key_up_event_sent = 0;
        }
        else
        {
            uint32_t press_duration = xTaskGetTickCount() - key_up_press_start;

            if (press_duration >= KEY_UP_HUMI_PRESS_TIME && !key_up_event_sent)
            {
                key_up_event_sent = 1;
                event = KEY_EVENT_UP_LONG_HUMI;
                BEEP_EnterHumiMode();
            }
            else if (press_duration >= KEY_UP_TEMP_PRESS_TIME && !key_up_event_sent)
            {
                key_up_event_sent = 1;
                event = KEY_EVENT_UP_LONG_TEMP;
                BEEP_EnterTempMode();
            }
        }
    }
    else
    {
        if (key_up_prev)
        {
            uint32_t press_duration = xTaskGetTickCount() - key_up_press_start;

            if (press_duration < KEY_UP_TEMP_PRESS_TIME && !key_up_event_sent)
            {
                event = KEY_EVENT_UP_SHORT;
                BEEP_Short(50);
            }
        }
    }
    key_up_prev = key_up_now;

    // ===== 2. 扫描 TPAD =====
    if (event == KEY_EVENT_NONE)
    {
        uint8_t tpad_now = TPAD_Scan();

        if (tpad_now && !tpad_prev)
        {
            event = KEY_EVENT_TPAD_TOUCH;
            BEEP_TPADTouch();
        }
        tpad_prev = tpad_now;
    }

    return event;
}

// ==================== 私有函数实现 ====================

static void TPAD_Init(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < 5; i++)
    {
        sum += TPAD_GetValue();
        osDelay(10);
    }
    tpad_default_val = sum / 5;
    tpad_init_done = 1;
}

static uint16_t TPAD_GetValue(void)
{
    uint16_t count = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    osDelay(1);

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    while (!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5))
    {
        count++;
        if (count > 5000)
            break;
    }

    return count;
}

static uint8_t TPAD_Scan(void)
{
    if (!tpad_init_done)
        return 0;

    uint16_t current_val = TPAD_GetValue();
    uint16_t diff = (current_val > tpad_default_val) ? (current_val - tpad_default_val) : (tpad_default_val - current_val);

    return (diff > TPAD_TOUCH_THRESHOLD) ? 1 : 0;
}
