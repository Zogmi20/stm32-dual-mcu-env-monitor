#ifndef DHT11_H
#define DHT11_H

#include "main.h"
#include "gpio.h"

// 根据你实际的 GPIO 修改
#define DHT11_PIN GPIO_PIN_1
#define DHT11_PORT GPIOA

// 函数声明
void DHT11_Init(void);
uint8_t DHT11_Read(float *temperature, float *humidity);

#endif
