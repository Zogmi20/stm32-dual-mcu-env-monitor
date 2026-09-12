#ifndef __RS485_H
#define __RS485_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// ==================== 硬件定义 ====================
#define RS485_UX USART2
#define RS485_TX_GPIO_PORT GPIOA
#define RS485_TX_GPIO_PIN GPIO_PIN_2
#define RS485_RX_GPIO_PORT GPIOA
#define RS485_RX_GPIO_PIN GPIO_PIN_3
#define RS485_RE_GPIO_PORT GPIOG
#define RS485_RE_GPIO_PIN GPIO_PIN_8

// ⚠️ 不需要时钟使能宏，因为 CubeMX 已经做了
// #define RS485_TX_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
// ...

#define RS485_RE(x) HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

#define RS485_REC_LEN 64

// ==================== 外部变量声明 ====================
extern uint8_t g_RS485_rx_buf[RS485_REC_LEN];
extern uint8_t g_RS485_rx_cnt;

// ==================== RS485 帧格式 ====================
typedef struct
{
    uint8_t header1; // 0xAA
    uint8_t header2; // 0x55
    float temp;
    float humi;
    uint8_t checksum;
} RS485_Frame_t;

// ==================== 函数声明 ====================
void rs485_init(uint32_t baudrate);
void rs485_send_data(uint8_t *buf, uint8_t len);
void rs485_receive_data(uint8_t *buf, uint8_t *len);
void rs485_send_frame(float temp, float humi);
void RS485_IRQHandler(void);
void StartTask_RS485(void const *argument);

#endif
