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
#include "lcd.h"
#include "flash.h"
#include "alarm.h"
#include "rs485.h"
#include "keytpad_input.h"


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
// 一条消息：存放温度、湿度
typedef struct
{
  float temp;
  float humi;
  uint8_t ok; // 1成功，0失败
} SensorMsg_t;


//====新增轮换缓冲区，3块足够
static SensorMsg_t gMsgPool[5];
static uint8_t poolIndex = 0;

//====温湿度独立报警阈值====
//====温湿度报警阈值 (大棚用)====
float temp_alarm_high = 35.0f; // 温度上限
float temp_alarm_low = 5.0f;   // 温度下限
float humi_alarm_high = 95.0f; // 湿度上限
float humi_alarm_low = 30.0f;  // 湿度下限
uint8_t set_mode_flag;

// 在文件顶部定义状态
typedef enum
{
  MODE_NORMAL = 0, // 正常显示模式
  MODE_SET_TEMP,   // 设置温度阈值模式
  MODE_SET_HUMI    // 设置湿度阈值模式
} SystemMode_t;

// 全局状态变量
// static SystemMode_t sys_mode = MODE_NORMAL;
// static uint8_t setting_choice = 0; // 0=温度, 1=湿度

//全局变量版本
// float g_temperature = 0.0f;
// float g_humidity = 0.0f;
// uint8_t g_sensor_ok = 0; // 0=失败, 1=成功

osMessageQId RS485QueueHandle;
/* USER CODE END Variables */
osThreadId Task_ReadSensorHandle;
osThreadId Task_DisplayHandle;
osThreadId Task_RS485Handle;
osMessageQId SensorQueueHandle;
osMutexId LcdMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartTask_ReadSensor(void const * argument);
void StartTask_Display(void const * argument);
void StartTask_RS485(void const * argument);

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
  /* Create the mutex(es) */
  /* definition and creation of LcdMutex */
  osMutexDef(LcdMutex);
  LcdMutexHandle = osMutexCreate(osMutex(LcdMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of SensorQueue */
  osMessageQDef(SensorQueue, 5, uint32_t);
  SensorQueueHandle = osMessageCreate(osMessageQ(SensorQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  // ===== 为 RS485 创建独立队列 =====
  osMessageQDef(RS485Queue, 5, uint32_t);
  RS485QueueHandle = osMessageCreate(osMessageQ(RS485Queue), NULL);

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Task_ReadSensor */
  osThreadDef(Task_ReadSensor, StartTask_ReadSensor, osPriorityHigh, 0, 512);
  Task_ReadSensorHandle = osThreadCreate(osThread(Task_ReadSensor), NULL);

  /* definition and creation of Task_Display */
  osThreadDef(Task_Display, StartTask_Display, osPriorityNormal, 0, 512);
  Task_DisplayHandle = osThreadCreate(osThread(Task_Display), NULL);

  /* definition and creation of Task_RS485 */
  osThreadDef(Task_RS485, StartTask_RS485, osPriorityNormal, 0, 512);
  Task_RS485Handle = osThreadCreate(osThread(Task_RS485), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

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
    // taskENTER_CRITICAL();//全局变量的互斥锁在队列不需要用
    ret = DHT11_Read(&temp, &humi);
    // taskEXIT_CRITICAL(); // 全局变量的互斥锁在队列不需要用
    // =================================================
    // 取一块全局缓存
    SensorMsg_t *pMsg = &gMsgPool[poolIndex];
    if (ret == 0)
    {
      // 全局变量版本
      //  g_temperature = temp;
      //  g_humidity = humi;
      //  g_sensor_ok = 1;
      // 取一块全局缓存
      pMsg->ok = 1;
      pMsg->temp = temp;
      pMsg->humi = humi;

      // 等待10个tick，队列满不会立刻丢弃
      osMessagePut(SensorQueueHandle, (uint32_t)pMsg, 10);
      osMessagePut(RS485QueueHandle, (uint32_t)pMsg, 10);

      sprintf(buffer, "Temp: %.1fC, Humi: %.1f%%\r\n", temp, humi);
      HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), 100);
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
    }
    else
    {
      // 全局变量版本
      //  g_sensor_ok = 0;
      // 读取失败！也发送消息，标记错误
      pMsg->ok = 0;
      osMessagePut(SensorQueueHandle, (uint32_t)pMsg, 10);
      osMessagePut(RS485QueueHandle, (uint32_t)pMsg, 10);
      HAL_UART_Transmit(&huart1, (uint8_t *)"Read Error\r\n", 13, 100);
      HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
    }
    // 轮换缓存下标
    poolIndex = (poolIndex + 1) % 5;
    // DHT11要求间隔 >= 1秒，这里用2秒
    osDelay(2000); // FreeRTOS的延时函数
  }
  /* USER CODE END StartTask_ReadSensor */
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
  char lcd_buf[32];
  osEvent evt;
  SensorMsg_t *pMsg;
// osStatus status;  // ← 注释掉未使用的变量

  // ============================================================
  // ===== 从 Flash 读取保存的阈值 =====
  // ============================================================
  float saved_temp_high, saved_temp_low, saved_humi_high, saved_humi_low;
  Flash_Read_All(&saved_temp_high, &saved_temp_low, &saved_humi_high, &saved_humi_low);

  // 赋值给全局变量（在 freertos.c 顶部定义的）
  temp_alarm_high = saved_temp_high;
  temp_alarm_low = saved_temp_low;
  humi_alarm_high = saved_humi_high;
  humi_alarm_low = saved_humi_low;

// ===== 呼吸灯参数 =====
#define BREATH_MAX 1000

  // ===== 呼吸灯变量 =====
  static uint16_t breath_pwm = 0;
  static uint8_t breath_dir = 1;

  lcd_init();
  lcd_clear(WHITE);

  // ===== 启动 PWM =====
  HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);

  // ===== 初始状态：红灯熄灭（低电平点亮 → 占空比 100% = 熄灭） =====
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 1000);

  lcd_show_string(20, 15, 200, 24, 24, "ENV MONITOR", BLUE);
  lcd_show_string(20, 274, 200, 16, 16, "PA0=SET PE2=ADD PE4=SUB", BLACK);

  for (;;)
  {
    evt = osMessageGet(SensorQueueHandle, osWaitForever);
    osMutexWait(LcdMutexHandle, osWaitForever);

    if (evt.status == osEventMessage)
    {
      pMsg = (SensorMsg_t *)evt.value.p;
      if (pMsg->ok == 1)
      {
        // ============================================================
        // ===== 🔧 第一步：检查是否处于设置模式（优先级最高） =====
        // ============================================================
        SetMode_t set_mode = Key_GetSetMode();

        if (set_mode != SET_MODE_NORMAL)
        {
          // ===== 静态变量记录上次的设置模式和值 =====
          static SetMode_t last_set_mode = SET_MODE_NORMAL;
          static uint16_t last_set_value = 0;

          uint16_t current_value = Key_GetSetValue();

          // ✅ 只有设置模式变化或值变化时才刷新
          if (set_mode != last_set_mode || current_value != last_set_value)
          {
            // ============================================================
            // ===== 设置模式界面 =====
            // ============================================================
            // lcd_fill(20, 40, 220, 64, WHITE);
            // lcd_fill(20, 80, 220, 104, WHITE);
            // lcd_fill(20, 120, 220, 144, WHITE);
            // lcd_fill(20, 160, 220, 184, WHITE);
            // lcd_fill(20, 200, 220, 224, WHITE);
            // 一次性清空 y=40~266 所有数据区域
            lcd_fill(20, 40, 220, 224, WHITE);

            const char *mode_str[] = {"NORMAL", "TEMP LOW", "TEMP HIGH", "HUMI LOW", "HUMI HIGH"};
            char set_buf[32];

            sprintf(set_buf, "SET: %s", mode_str[set_mode]);
            lcd_show_string(20, 40, 200, 24, 24, set_buf, BLUE);

            sprintf(set_buf, "Value: %d", Key_GetSetValue());
            lcd_show_string(20, 80, 200, 24, 24, set_buf, BLUE);

            lcd_show_string(20, 250, 200, 16, 16, "Long PA0=Cancel", BLACK);
            // ✅ 更新记录
            last_set_mode = set_mode;
            last_set_value = current_value;
          }
            // ===== 设置模式下：屏蔽报警（红灯熄灭） =====
            HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
            __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, BREATH_MAX);

            // 跳过正常模式的报警检测和呼吸灯
            // 直接跳到 osMutexRelease
          }
        else
        {
          // ============================================================
          // ===== 正常模式 =====
          // ============================================================
          // ===== ✅ 定义静态变量（函数开头） =====
          static float last_temp = 0;
          static float last_humi = 0;
          static uint8_t last_alarm = 0xFF;

          // ===== ✅ 判断是否需要刷新 =====
          uint8_t alarm_state = Alarm_IsTriggered();
          uint8_t need_refresh = 0;

          if (pMsg->temp != last_temp || pMsg->humi != last_humi)
          {
            need_refresh = 1;
          }
          if (alarm_state != last_alarm)
          {
            need_refresh = 1;
          }
          // ===== ✅ 只有变化时才需要刷新 =====
          if (need_refresh)
          {
            // ===== LCD 显示 =====
            // lcd_fill(20, 40, 220, 64, WHITE);
            // lcd_fill(20, 80, 220, 104, WHITE);
            // lcd_fill(20, 120, 220, 144, WHITE);
            // lcd_fill(20, 160, 220, 184, WHITE);
            // lcd_fill(20, 250, 220, 266, WHITE);
            // 一次性清空 y=40~266 所有数据区域
            lcd_fill(20, 40, 220, 266, WHITE);

            sprintf(lcd_buf, "Temp:%.1f C", pMsg->temp);
            lcd_show_string(20, 40, 200, 24, 24, lcd_buf, BLACK);

            sprintf(lcd_buf, "Humi:%.1f %%", pMsg->humi);
            lcd_show_string(20, 80, 200, 24, 24, lcd_buf, BLACK);

            sprintf(lcd_buf, "T:%.1f~%.1f C", temp_alarm_low, temp_alarm_high);
            lcd_show_string(20, 120, 200, 24, 24, lcd_buf, BLACK);

            sprintf(lcd_buf, "H:%.1f~%.1f %%", humi_alarm_low, humi_alarm_high);
            lcd_show_string(20, 160, 200, 24, 24, lcd_buf, BLACK);
            // ===== ✅ 更新记录 =====
            last_temp = pMsg->temp;
            last_humi = pMsg->humi;
            last_alarm = alarm_state;
          }
            // ===== 报警检测 =====
            Alarm_Update(pMsg->temp, pMsg->humi,
                         temp_alarm_high, temp_alarm_low,
                         humi_alarm_high, humi_alarm_low);

            // ============================================================
            // ===== 🔥 红灯控制 =====
            // ============================================================
            if (Alarm_IsTriggered())
            {
              // ===== 🚨 报警：温度越界越严重，呼吸越快 =====
              HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_SET);

              // ---- 计算温度越界程度 ----
              float over_ratio = 0.0f;
              float temp = pMsg->temp;

              if (temp > temp_alarm_high)
              {
                over_ratio = (temp - temp_alarm_high) / (temp_alarm_high * 0.3f);
              }
              else if (temp < temp_alarm_low)
              {
                over_ratio = (temp_alarm_low - temp) / (temp_alarm_low * 0.3f);
              }

              if (over_ratio < 0)
                over_ratio = 0;
              if (over_ratio > 1)
                over_ratio = 1;

              // ---- 越界程度 → 呼吸步长（10~100） ----
              uint16_t dynamic_step = 10 + (uint16_t)(over_ratio * 90);

              // ---- 呼吸算法（用动态步长） ----
              if (breath_dir)
              {
                breath_pwm += dynamic_step;
                if (breath_pwm >= BREATH_MAX)
                {
                  breath_pwm = BREATH_MAX;
                  breath_dir = 0;
                }
              }
              else
              {
                if (breath_pwm > dynamic_step)
                {
                  breath_pwm -= dynamic_step;
                }
                else
                {
                  breath_pwm = 0;
                  breath_dir = 1;
                }
              }

              __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, BREATH_MAX - breath_pwm);

              lcd_fill(20, 200, 220, 224, WHITE);

              // 获取报警类型
              uint8_t alarm_type = Alarm_GetType();
              const char *type_str[] = {"Recovered", "TEMP HIGH", "TEMP LOW", "HUMI HIGH", "HUMI LOW", "BOTH"};

              char alarm_buf[32];
              sprintf(alarm_buf, "ALARM:[%s]", type_str[alarm_type]);
              lcd_show_string(20, 200, 200, 24, 24, alarm_buf, RED);
            }
            else
            {
              // ===== ✅ 正常：红灯熄灭 =====
              HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);
              __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, BREATH_MAX);

              lcd_fill(20, 200, 240, 224, WHITE);

              AlarmState_t state = Alarm_GetState();
              if (state == ALARM_STATE_WARNING)
              {
                uint8_t cnt = Alarm_GetContinuousCount();
                sprintf(lcd_buf, "Warning:%d/5", cnt);
                lcd_show_string(20, 200, 200, 24, 24, lcd_buf, BLUE);
              }
              else if (state == ALARM_STATE_CLEARING)
              {
                uint8_t cnt = Alarm_GetNormalCount();
                sprintf(lcd_buf, "Clearing:%d/3", cnt);
                lcd_show_string(20, 200, 200, 24, 24, lcd_buf, GREEN);
              }
              else
              {
                lcd_show_string(20, 200, 200, 24, 24, "Temp Humi Normal", BLACK);
              }
            }
          } // ===== 正常模式结束 =====
      }
      else
      {
        // ===== 传感器错误 =====
        lcd_fill(20, 40, 220, 64, WHITE);
        lcd_fill(20, 80, 220, 104, WHITE);
        lcd_show_string(20, 40, 220, 24, 24, "Sensor Error!", BLACK);

        HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8, GPIO_PIN_RESET);

        // 传感器错误：红灯常亮（0 = 最亮）
        __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, 0);
      }
    }

    osMutexRelease(LcdMutexHandle);
    // ===== 按键扫描（互斥锁释放之后） =====
    Key_Process();
    osDelay(200);
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
  osEvent evt;
  SensorMsg_t *pMsg;

  // 初始化 RS485 (使用 USART2)
  rs485_init(115200);

  /* Infinite loop */
  for (;;)
  {
    // ===== ✅ 从专用队列阻塞等待 =====
    evt = osMessageGet(RS485QueueHandle, osWaitForever);

    if (evt.status == osEventMessage)
    {
      pMsg = (SensorMsg_t *)evt.value.p;
      if (pMsg->ok == 1)
      {
        // 立即发送 RS485 数据
        rs485_send_frame(pMsg->temp, pMsg->humi);

        // 串口调试输出
        char buf[64];
        sprintf(buf, "RS485 Send: T=%.1f H=%.1f\r\n", pMsg->temp, pMsg->humi);
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
      }
    }

    // 不需要 osDelay，有消息就立即处理，没消息就阻塞等待
  }
  /* USER CODE END StartTask_RS485 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
