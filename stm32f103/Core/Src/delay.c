#include "delay.h"
#include "tim.h" // 引用外部声明的 htim2

extern TIM_HandleTypeDef htim2;
// 微秒延时（基于TIM2，CNT每1us加1）
void delay_us(uint32_t us)
{
    // 读取当前计数值作为起始时间
    uint32_t start = TIM2->CNT;
    uint32_t target = start + us;

    // 等待计数值达到目标
    // 注意：TIM2是16位计数器，最大65535，需要处理溢出
    if (target > 65535)
    {
        // 需要等待溢出
        while (TIM2->CNT > start)
            ; // 等待溢出（从65535→0）
        while (TIM2->CNT < (target - 65535))
            ; // 等待剩余部分
    }
    else
    {
        while (TIM2->CNT < target)
            ;
    }
}

