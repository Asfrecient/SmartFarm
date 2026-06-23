/**
 * @file SensorTask.c
 * @brief 传感器数据采集和报警检测任务
 * 
 * 本任务负责：
 * 1. 初始化所有传感器（AHT20温湿度、土壤湿度、光照、降雨量）
 * 2. 定期读取传感器数据并更新全局状态
 * 3. 检查环境参数是否超出安全范围
 * 4. 当检测到异常时启动蜂鸣器
 * 
 * 任务优先级：osPriorityNormal
 * 任务周期：1000ms（1秒）
 */

#include "aht20/aht20.h"
#include "cmsis_os2.h"
#include "freertos.h"
#include "global/farmState.h"
#include "light/light.h"
#include "main.h"
#include "rain/rain.h"
#include "soil_moisture/soil_moisture.h"
#include "pump/pump.h"
#include <stdint.h>
#include "BeepTimer.h"
/**
 * @brief 检查浮点数是否在安全范围内
 * 
 * 如果值超出范围，则返回报警标志
 * 
 * @param value 要检查的浮点数值
 * @param min 最小值阈值
 * @param max 最大值阈值
 * @param minReason 低于最小值时的报警原因字符串
 * @param maxReason 高于最大值时的报警原因字符串
 * @return 返回1表示有报警，返回0表示正常
 */
static uint8_t CheckRangeFloat(float value, float min, float max, const char *minReason, const char *maxReason) {
  (void)minReason;
  (void)maxReason;
  uint8_t warning = 0;
  
  // 检查是否低于最小值
  if (value < min) {
    warning = 1;
  }
  
  // 检查是否高于最大值
  if (value > max) {
    warning = 1;
  }
  
  return warning;
}

/**
 * @brief 检查整数是否在安全范围内
 * 
 * 如果值超出范围，则返回报警标志
 * 
 * @param value 要检查的整数值
 * @param min 最小值阈值
 * @param max 最大值阈值
 * @param minReason 低于最小值时的报警原因字符串
 * @param maxReason 高于最大值时的报警原因字符串
 * @return 返回1表示有报警，返回0表示正常
 */
static uint8_t CheckRangeInt(int value, int min, int max, const char *minReason, const char *maxReason) {
  (void)minReason;
  (void)maxReason;
  uint8_t warning = 0;
  
  // 检查是否低于最小值
  if (value < min) {
    warning = 1;
  }
  
  // 检查是否高于最大值
  if (value > max) {
    warning = 1;
  }
  
  return warning;
}

/**
 * @brief 传感器任务主函数
 * 
 * 任务执行流程：
 * 1. 初始化环境安全范围阈值
 * 2. 初始化所有传感器（需要互斥锁保护I2C总线）
 * 3. 进入主循环：
 *    - 读取所有传感器数据
 *    - 更新全局状态变量
 *    - 检查各参数是否超出安全范围
 *    - 如有异常，启动蜂鸣器
 *    - 延时1秒后继续下一次循环
 * 
 * @param argument 任务参数（未使用）
 * 
 * @note 
 * - 使用i2c1Mutex互斥锁保护I2C1总线，因为OLED和AHT20共享I2C1
 * - 任务周期为1秒，保证数据更新及时且不会过于频繁
 */
void StartSensorTask(void *argument) {
  // 初始化环境安全范围阈值（设置默认值）
  EnvSafeRange_Init();
  
  // 初始化AHT20温湿度传感器（需要I2C通信，使用互斥锁保护）
  osMutexAcquire(i2c1MutexHandle, osWaitForever);
  AHT20_Init();
  osMutexRelease(i2c1MutexHandle);
  
  // 初始化其他传感器（使用ADC，不需要I2C）
  Rain_init();           // 降雨量传感器
  SoilMoisture_init();   // 土壤湿度传感器
  
  // 主循环：定期采集传感器数据并检测报警
  for (;;) {
    // 读取AHT20温湿度传感器数据（需要I2C通信，使用互斥锁保护）
    osMutexAcquire(i2c1MutexHandle, osWaitForever);
    AHT20_Read(&farmState.temperature, &farmState.humidity);
    osMutexRelease(i2c1MutexHandle);
    
    // 读取其他传感器数据（使用ADC，不需要互斥锁）
    farmState.rainGauge = Rain_Get();           // 读取降雨量
    farmState.soilMoisture = SoilMoisture_Get(); // 读取土壤湿度
    farmState.lightIntensity = Light_Get();     // 读取光照强度
    
    // 根据土壤湿度自动控制水泵
    // 当土壤湿度低于最低阈值时打开水泵，高于最低阈值时关闭水泵
    if (farmState.soilMoisture < farmSafeRange.minSoilMoisture) {
      // 土壤湿度低于最低阈值，打开水泵
      // Pump_On();
      farmState.waterPumpState = 1;
    } else {
      // 土壤湿度高于或等于最低阈值，关闭水泵
      Pump_Off();
      farmState.waterPumpState = 0;
    }
    
    // 检查各环境参数是否超出安全范围，累计报警标志
    uint8_t warning = 0;
    warning += CheckRangeFloat(farmState.temperature, farmSafeRange.minTemperature, 
                               farmSafeRange.maxTemperature, "temperature_low", "temperature_high");
    warning += CheckRangeFloat(farmState.humidity, farmSafeRange.minHumidity, 
                               farmSafeRange.maxHumidity, "humidity_low", "humidity_high");
    
    // 降雨量只有上限检查（超过阈值才报警）
    if (farmState.rainGauge > farmSafeRange.maxRainGauge) {
      warning += 1;
    }

    warning += CheckRangeInt(farmState.soilMoisture, farmSafeRange.minSoilMoisture,
                             farmSafeRange.maxSoilMoisture, "soil_moisture_low", "soil_moisture_high");
    warning += CheckRangeInt(farmState.lightIntensity, farmSafeRange.minLightIntensity,
                             farmSafeRange.maxLightIntensity, "light_intensity_low", "light_intensity_high");
   
    // 根据报警标志控制蜂鸣器
    if (warning > 0) {
      // Beep_on();  // 有报警，启动蜂鸣器
    } else {
      Beep_off(); // 无报警，关闭蜂鸣器
    }
    
    // 延时1秒后继续下一次数据采集
    osDelay(1000);
  }
}
