/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "delay.h"
#include "main.h"
#include "dht11.h"
#include "usart.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
float g_temperature = 0.0f;
float g_humidity = 0.0f;
uint8_t g_sensor_ok = 0; // 0=失败, 1=成功
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId Task_DisplayHandle;
osThreadId Task_RS485Handle;
osThreadId Task_ReadSensorHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartTask_Display(void const * argument);
void StartTask_RS485(void const * argument);
void StartTask_ReadSensor(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of Task_Display */
  osThreadDef(Task_Display, StartTask_Display, osPriorityNormal, 0, 512);
  Task_DisplayHandle = osThreadCreate(osThread(Task_Display), NULL);

  /* definition and creation of Task_RS485 */
  osThreadDef(Task_RS485, StartTask_RS485, osPriorityNormal, 0, 512);
  Task_RS485Handle = osThreadCreate(osThread(Task_RS485), NULL);

  /* definition and creation of Task_ReadSensor */
  osThreadDef(Task_ReadSensor, StartTask_ReadSensor, osPriorityHigh, 0, 512);
  Task_ReadSensorHandle = osThreadCreate(osThread(Task_ReadSensor), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    // HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_9);
    // printf("Hello World!\r\n");
    // osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask_Display */
/**
* @brief Function implementing the Task_Display thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_Display */
void StartTask_Display(void const * argument)
{
  /* USER CODE BEGIN StartTask_Display */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask_Display */
}

/* USER CODE BEGIN Header_StartTask_RS485 */
/**
* @brief Function implementing the Task_RS485 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_RS485 */
void StartTask_RS485(void const * argument)
{
  /* USER CODE BEGIN StartTask_RS485 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTask_RS485 */
}

/* USER CODE BEGIN Header_StartTask_ReadSensor */
/**
* @brief Function implementing the Task_ReadSensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask_ReadSensor */
void StartTask_ReadSensor(void const * argument)
{
  /* USER CODE BEGIN StartTask_ReadSensor */
  float temp = 0.0f, humi = 0.0f;
  char buffer[64];
  uint8_t ret;
  DHT11_Init(); // ✅DHT11硬件初始化，仅执行一次！！

  /* Infinite loop */
  for(;;)
  {
    //================================ DHT11温湿度传感器
    // ===== 临界区保护 DHT11 读取（禁用任务切换） =====
    taskENTER_CRITICAL();
    ret = DHT11_Read(&temp, &humi);
    taskEXIT_CRITICAL();
    // =================================================

    if (ret == 0)
    {
      g_temperature = temp;
      g_humidity = humi;
      g_sensor_ok = 1;

      sprintf(buffer, "Temp: %.1fC, Humi: %.1f%%\r\n", temp, humi);
      HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), 100);
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);
    }
    else
    {
      g_sensor_ok = 0;
      HAL_UART_Transmit(&huart1, (uint8_t *)"Read Error\r\n", 13, 100);
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
    }

    // DHT11要求间隔 >= 1秒，这里用2秒
    osDelay(2000); // FreeRTOS的延时函数
  }
  /* USER CODE END StartTask_ReadSensor */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
