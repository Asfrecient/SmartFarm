/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "aht20/aht20.h"
#include "oled/oled.h"
#include "oled/font.h"
#include "lora_e32.h"
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

/* USER CODE END Variables */

/* ============================================================================
 * FreeRTOS资源定义
 * ============================================================================ */

/* SensorTask任务定义
 * 功能：传感器数据采集和报警检测
 * 优先级：osPriorityNormal（普通优先级）
 * 堆栈大小：256 * 4 = 1024字节（足够存储局部变量和函数调用栈）
 */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,  // 1024字节堆栈
  .priority = (osPriority_t) osPriorityNormal,  // 普通优先级
};

/* InputTask任务定义
 * 功能：用户输入处理（按键、旋钮）
 * 优先级：osPriorityHigh（高优先级，保证用户输入响应及时）
 * 堆栈大小：128 * 4 = 512字节
 */
osThreadId_t InputTaskHandle;
const osThreadAttr_t InputTask_attributes = {
  .name = "InputTask",
  .stack_size = 128 * 4,  // 512字节堆栈
  .priority = (osPriority_t) osPriorityHigh,  // 高优先级
};

/* ScreenTask任务定义
 * 功能：OLED屏幕显示
 * 优先级：osPriorityBelowNormal5（低优先级，不影响实时性要求高的任务）
 * 堆栈大小：128 * 4 = 512字节
 */
osThreadId_t ScreenTaskHandle;
const osThreadAttr_t ScreenTask_attributes = {
  .name = "ScreenTask",
  .stack_size = 128 * 4,  // 512字节堆栈
  .priority = (osPriority_t) osPriorityBelowNormal5,  // 低优先级
};

/* LoRaTask任务定义
 * 功能：LoRa通信（通过UART3向E32模块周期发送农场数据）
 * 优先级：osPriorityLow（低优先级，不影响实时性要求高的任务）
 * 堆栈大小：128 * 4 = 512字节
 */
osThreadId_t LoRaTaskHandle;
const osThreadAttr_t LoRaTask_attributes = {
  .name = "LoRaTask",
  .stack_size = 128 * 4,  // 512字节堆栈
  .priority = (osPriority_t) osPriorityLow,  // 低优先级
};

/* BeepTimer软件定时器定义
 * 用途：控制蜂鸣器的周期性响铃
 * 类型：周期性定时器（osTimerPeriodic）
 * 周期：500ms（在Beep_on()中设置）
 */
osTimerId_t BeepTimerHandle;
const osTimerAttr_t BeepTimer_attributes = {
  .name = "BeepTimer"
};

/* i2c1Mutex互斥锁定义
 * 用途：保护I2C1总线，防止OLED和AHT20同时访问I2C1造成冲突
 * 使用场景：
 *   - SensorTask读取AHT20时需要获取互斥锁
 *   - ScreenTask刷新OLED显示时需要获取互斥锁
 */
osMutexId_t i2c1MutexHandle;
const osMutexAttr_t i2c1Mutex_attributes = {
  .name = "i2c1Mutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSensorTask(void *argument);
extern void StartInputTask(void *argument);
extern void StartScreenTask(void *argument);
extern void LoRa_Task(void *argument);
extern void BeepTimerCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS初始化函数
  * 
  * 本函数在系统启动时调用，负责创建所有FreeRTOS资源：
  * 1. 互斥锁（Mutex）：保护共享资源（I2C1总线）
  * 2. 软件定时器（Timer）：控制蜂鸣器周期性响铃
  * 3. 消息队列（Queue）：任务间通信（报警消息传递）
  * 4. 任务（Thread）：创建所有应用任务
  * 
  * 创建顺序说明：
  * - 先创建同步原语（互斥锁、信号量等）
  * - 再创建通信机制（队列、事件组等）
  * - 最后创建任务（任务创建后立即进入就绪态，等待调度器启动）
  * 
  * @param  None
  * @retval None
  * 
  * @note 本函数在osKernelStart()之前调用，此时调度器尚未启动
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  
  /* ========================================================================
   * 创建互斥锁（Mutex）
   * ======================================================================== */
  /* 创建i2c1Mutex互斥锁
   * 用途：保护I2C1总线，防止OLED和AHT20同时访问造成冲突
   * 使用场景：
   *   - SensorTask读取AHT20前获取，读取后释放
   *   - ScreenTask刷新OLED前获取，刷新后释放
   */
  i2c1MutexHandle = osMutexNew(&i2c1Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* ========================================================================
   * 创建软件定时器（Timer）
   * ======================================================================== */
  /* 创建BeepTimer软件定时器
   * 类型：周期性定时器（osTimerPeriodic）
   * 回调函数：BeepTimerCallback（实现蜂鸣器开关切换）
   * 周期：500ms（在Beep_on()中通过osTimerStart设置）
   * 初始状态：停止（需要调用Beep_on()才会启动）
   */
  BeepTimerHandle = osTimerNew(BeepTimerCallback, osTimerPeriodic, NULL, &BeepTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* ========================================================================
   * 创建任务（Thread）
   * ======================================================================== */
  /* 创建SensorTask任务
   * 功能：传感器数据采集和报警检测
   * 优先级：osPriorityNormal
   * 堆栈大小：1024字节
   * 执行周期：1000ms（1秒采集一次）
   */
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  /* 创建InputTask任务
   * 功能：用户输入处理（按键、旋钮）
   * 优先级：osPriorityHigh（高优先级，保证响应及时）
   * 堆栈大小：512字节
   * 执行周期：10ms（快速响应）
   */
  InputTaskHandle = osThreadNew(StartInputTask, NULL, &InputTask_attributes);

  /* 创建ScreenTask任务
   * 功能：OLED屏幕显示
   * 优先级：osPriorityBelowNormal5（低优先级）
   * 堆栈大小：512字节
   * 执行周期：10ms（保证显示流畅）
   */
  ScreenTaskHandle = osThreadNew(StartScreenTask, NULL, &ScreenTask_attributes);

  /* 创建LoRaTask任务
   * 功能：LoRa通信（通过UART3向E32模块周期发送农场数据）
   * 优先级：osPriorityLow（低优先级）
   * 堆栈大小：512字节
   * 执行方式：周期发送数据
   */
  LoRaTaskHandle = osThreadNew(LoRa_Task, NULL, &LoRaTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartSensorTask */
/**
  * @brief  Function implementing the SensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSensorTask */
__weak void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartSensorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

