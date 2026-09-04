#include "rs485.h"
#include "usart.h"
#include "app_type.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>


// ==================== 外部变量 ====================
extern UART_HandleTypeDef huart2; // ← 使用 CubeMX 生成的
extern osMessageQId SensorQueueHandle;

// ==================== 私有变量 ====================
uint8_t g_RS485_rx_buf[RS485_REC_LEN];
uint8_t g_RS485_rx_cnt = 0;
uint8_t g_RS485_tx_buf[RS485_REC_LEN];
// ? 删除：UART_HandleTypeDef g_rs485_uart;

// ==================== 1. 初始化 ====================
void rs485_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct;

    // 时钟已在 MX_USART2_UART_Init 中使能，不需要重复
    // RS485_TX_GPIO_CLK_ENABLE();
    // RS485_RX_GPIO_CLK_ENABLE();
    // RS485_UX_CLK_ENABLE();

    // 配置控制引脚 RE (PG8)
    gpio_init_struct.Pin = RS485_RE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_RE_GPIO_PORT, &gpio_init_struct);
    RS485_RE(0);

    // 如果要改波特率，需要重新调用 HAL_UART_Init
    if (baudrate != 115200)
    {
        huart2.Init.BaudRate = baudrate;
        HAL_UART_Init(&huart2);
    }
    // ===== 启动 DMA 接收 =====
    HAL_UART_Receive_DMA(&huart2, g_RS485_rx_buf, RS485_REC_LEN);

    // ===== 使能空闲中断 =====
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);

}

// ==================== 2. 发送数据 ====================
void rs485_send_data(uint8_t *buf, uint8_t len)
{
// 如果启用调试，打印发送数据
#if 0 // 改为 1 开启调试
    char dbg[64];
    sprintf(dbg, "Send %d: ", len);
    HAL_UART_Transmit(&huart1, (uint8_t *)dbg, strlen(dbg), 100);
    for (int i = 0; i < len; i++) {
        sprintf(dbg, "%02X ", buf[i]);
        HAL_UART_Transmit(&huart1, (uint8_t *)dbg, strlen(dbg), 100);
    }
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 100);
#endif

    RS485_RE(1);                               // 发送模式
    HAL_UART_Transmit(&huart2, buf, len, 100); // 使用 huart2
    osDelay(1);
    RS485_RE(0); // 接收模式
}

// ==================== 3. 发送帧 ====================
void rs485_send_frame(float temp, float humi)
{
    RS485_Frame_t frame;
    uint8_t *p = (uint8_t *)&frame;
    uint8_t sum = 0;
    int i;

    // 清空结构体
    memset(&frame, 0, sizeof(RS485_Frame_t));

    frame.header1 = 0xAA;
    frame.header2 = 0x55;
    frame.temp = temp;
    frame.humi = humi;

    for (i = 0; i < sizeof(RS485_Frame_t) - 1; i++)
    {
        sum += p[i];
    }
    frame.checksum = sum;

    rs485_send_data((uint8_t *)&frame, sizeof(RS485_Frame_t));
}

// ==================== 4. 接收数据 ====================
void rs485_receive_data(uint8_t *buf, uint8_t *len)
{
    *len = g_RS485_rx_cnt;
    for (int i = 0; i < g_RS485_rx_cnt; i++)
    {
        buf[i] = g_RS485_rx_buf[i];
    }
    g_RS485_rx_cnt = 0;
}

// ==================== 5. 串口空闲中断 ====================
// ===== RS485 空闲中断处理函数 =====
void RS485_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE))
    {
        // 清除空闲中断标志
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);

        // 停止 DMA 接收，获取已接收数据长度
        HAL_UART_DMAStop(&huart2);

        // 计算接收到的数据长度
        g_RS485_rx_cnt = RS485_REC_LEN - __HAL_DMA_GET_COUNTER(huart2.hdmarx);

        // 重新启动 DMA 接收
        HAL_UART_Receive_DMA(&huart2, g_RS485_rx_buf, RS485_REC_LEN);

        // 如果有数据，可以在这里处理
        if (g_RS485_rx_cnt > 0)
        {
            // 可添加帧解析逻辑
            // rs485_process_frame(g_RS485_rx_buf, g_RS485_rx_cnt);
        }
    }
}


