#include "main.h"
#include "dht11.h"
#include "usart.h"
#include "delay.h"
#include "string.h"
#include "stdio.h"

// ===== 重试次数配置 =====
#define DHT11_MAX_RETRY    10      // 最大重试次数
#define DHT11_TIMEOUT_US   300     // 超时时间（微秒）

// ===== DHT11 初始化 =====
void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
}

// ===== 等待引脚电平（带超时） =====
static uint8_t DHT11_WaitPinLevel(uint32_t level, uint32_t timeout_us)
{
    uint32_t count = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != level)
    {
        delay_us(1);
        count++;
        if (count >= timeout_us) return 1;
    }
    return 0;
}

// ===== 读取一个位（测量高电平持续时间） =====
static uint8_t DHT11_ReadBit(uint8_t *bit)
{
    uint32_t high_time = 0;
    
    // 1. 等待低电平开始
    if (DHT11_WaitPinLevel(GPIO_PIN_RESET, DHT11_TIMEOUT_US) != 0) return 1;
    
    // 2. 等待低电平结束（约50us）
    if (DHT11_WaitPinLevel(GPIO_PIN_SET, DHT11_TIMEOUT_US) != 0) return 1;
    
    // 3. 测量高电平持续时间
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        high_time++;
        delay_us(1);
        if (high_time > DHT11_TIMEOUT_US) return 1;
    }
    
    // 4. 判断：高电平 > 30us 表示 '1'，否则 '0'
    *bit = (high_time > 30) ? 1 : 0;
    
    return 0;
}

// ===== 读取一个字节 =====
static uint8_t DHT11_ReadByte(uint8_t *byte)
{
    uint8_t bit = 0;
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++)
    {
        if (DHT11_ReadBit(&bit) != 0) return 1;
        data = (data << 1) | bit;
    }
    
    *byte = data;
    return 0;
}

// ===== 单次读取（内部函数） =====
static uint8_t DHT11_ReadOnce(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    uint32_t retry = 0;
    
    // 1. 发起启动信号
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(30);
    
    // 2. 等待DHT11响应（拉低）
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        retry++;
        delay_us(1);
        if (retry > DHT11_TIMEOUT_US) return 1;
    }
    
    // 等待响应低电平结束
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET)
    {
        retry++;
        delay_us(1);
        if (retry > DHT11_TIMEOUT_US) return 1;
    }
    
    // 等待响应高电平结束
    retry = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
    {
        retry++;
        delay_us(1);
        if (retry > DHT11_TIMEOUT_US) return 1;
    }
    
    // 3. 读取5个字节
    for (int i = 0; i < 5; i++)
    {
        if (DHT11_ReadByte(&data[i]) != 0) return 1;
    }
    
    // 4. 校验和检查
    if (data[4] != data[0] + data[1] + data[2] + data[3]) return 1;
    
    // 5. 数据范围检查（防止读到错误数据）
    float h = data[0] + data[1] * 0.1f;
    float t = data[2] + data[3] * 0.1f;
    
    if (h < 0 || h > 100 || t < -20 || t > 80) return 1;
    
    *humidity = h;
    *temperature = t;
    
    return 0;
}

// ===== 对外接口：带容错重试 =====
uint8_t DHT11_Read(float *temperature, float *humidity)
{
    uint8_t retry_count = 0;
    uint8_t result = 1;
    
    for (retry_count = 0; retry_count < DHT11_MAX_RETRY; retry_count++)
    {
        result = DHT11_ReadOnce(temperature, humidity);
        
        if (result == 0)
        {
            // 读取成功，直接返回
            return 0;
        }
        
        // 失败后等待一小段时间再重试
        delay_us(100); // 等待100us
    }
    
    // 所有重试都失败
    return 1;
}
