/**
 * @file ScreenTask.c
 * @brief OLED屏幕显示任务
 * 
 * 本任务负责：
 * 1. 初始化OLED显示屏
 * 2. 根据当前页面索引渲染相应的界面
 * 3. 定期刷新显示内容
 * 
 * 任务优先级：osPriorityBelowNormal5（低优先级，不影响实时性要求高的任务）
 * 任务周期：10ms（保证显示流畅）
 * 
 * 显示页面：
 * - 首页（PAGE_HOME）：显示当前环境状态数据
 * - 阈值设置页（PAGE_RANGE）：显示和编辑报警阈值
 * 
 * @note 使用i2c1Mutex互斥锁保护I2C1总线，因为OLED和AHT20共享I2C1
 */

#include "oled/font.h"
#include "oled/oled.h"
#include "cmsis_os.h"
#include "global/farmState.h"
#include "global/screen.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "utils.h"

/**
 * @brief 计算居中文字的开始X坐标
 * 
 * 根据字符串长度和字体宽度，计算使文字在指定X坐标居中的起始X坐标
 * 
 * @param str 要显示的字符串
 * @param x 居中的目标X坐标（屏幕中心点）
 * @param width 单个字符的宽度（像素）
 * @return 计算得到的起始X坐标
 */
uint8_t getCenteredX(const char *str, uint8_t x, uint8_t width) {
  const uint8_t len = strlen(str);
  return x - (len * width) / 2;
}
/**
 * @brief 渲染首页界面
 * 
 * 显示当前环境状态数据，包括：
 * - 标题栏："Smart Farm"
 * - 第一行：温度、湿度、光照强度
 * - 第二行：土壤湿度、降雨量、水泵状态
 * 
 * 布局说明：
 * - 屏幕尺寸：128x64像素
 * - 使用12x12字体显示中文标签和数值
 * - 数值居中显示在对应区域
 */
void renderHomePage() {
  char msg[16];
  uint8_t x;

  // 显示标题栏（反色显示）
  OLED_PrintASCIIString(21, 0, " LoRa Smart Farm ", &afont12x6, OLED_COLOR_REVERSED);
  
  // 第一行：温度、湿度、光照强度
  // 温度显示
  OLED_PrintString(9, 14, "温度", &font12x12, OLED_COLOR_NORMAL);
  int minInt, minDec;
  floatToIntDec(farmState.temperature, &minInt, &minDec);
  sprintf(msg, "%d.%d", minInt, minDec);
  x = getCenteredX(msg, 21, 6);  // 计算居中位置（21是温度区域中心X坐标）
  sprintf(msg, "%d.%d℃", minInt, minDec);
  x = x - 6;  // 减去"℃"符号的宽度
  OLED_PrintString(x, 26, msg, &font12x12, OLED_COLOR_NORMAL);

  // 湿度显示
  OLED_PrintString(52, 14, "湿度", &font12x12, OLED_COLOR_NORMAL);
  floatToIntDec(farmState.humidity, &minInt, &minDec);
  sprintf(msg, "%d.%d%%", minInt, minDec);
  x = getCenteredX(msg, 64, 6);  // 64是湿度区域中心X坐标
  OLED_PrintString(x, 26, msg, &font12x12, OLED_COLOR_NORMAL);

  // 光照强度显示
  OLED_PrintString(95, 14, "光照", &font12x12, OLED_COLOR_NORMAL);
  sprintf(msg, "%d lx", farmState.lightIntensity);
  x = getCenteredX(msg, 107, 6);  // 107是光照区域中心X坐标
  OLED_PrintString(x, 26, msg, &font12x12, OLED_COLOR_NORMAL);

  // 第二行：土壤湿度、降雨量、水泵状态
  // 土壤湿度显示
  OLED_PrintString(9, 41, "土壤", &font12x12, OLED_COLOR_NORMAL);
  sprintf(msg, "%d%%", farmState.soilMoisture);
  x = getCenteredX(msg, 21, 6);
  OLED_PrintString(x, 52, msg, &font12x12, OLED_COLOR_NORMAL);

  // 降雨量显示
  OLED_PrintString(52, 41, "降雨", &font12x12, OLED_COLOR_NORMAL);
  sprintf(msg, "%d%%", farmState.rainGauge);
  x = getCenteredX(msg, 64, 6);
  OLED_PrintString(x, 52, msg, &font12x12, OLED_COLOR_NORMAL);

  // 水泵状态显示
  OLED_PrintString(95, 41, "水泵", &font12x12, OLED_COLOR_NORMAL);
  if (farmState.waterPumpState) {
    OLED_PrintString(101, 52, "开", &font12x12, OLED_COLOR_NORMAL);
  } else {
    OLED_PrintString(101, 52, "关", &font12x12, OLED_COLOR_NORMAL);
  }
}

// 阈值设置页的布局宏定义
#define RANGE_LINE_HEIGHT 12        // 每行文字的高度（像素）
#define RANGE_TEMPERATURE_Y 14      // 温度行的Y坐标
#define RANGE_TEMPERATURE_LINT_Y (RANGE_TEMPERATURE_Y + RANGE_LINE_HEIGHT -1)  // 温度行下划线Y坐标
#define RANGE_HUMIDITY_Y (RANGE_TEMPERATURE_Y + RANGE_LINE_HEIGHT + 5)  // 湿度行的Y坐标
#define RANGE_HUMIDITY_LINT_Y (RANGE_HUMIDITY_Y + RANGE_LINE_HEIGHT -1)  // 湿度行下划线Y坐标
#define RANGE_LIGHT_INTENSITY_Y (RANGE_HUMIDITY_Y + RANGE_LINE_HEIGHT + 5)  // 光照强度行的Y坐标
#define RANGE_LIGHT_INTENSITY_LINT_Y (RANGE_LIGHT_INTENSITY_Y + RANGE_LINE_HEIGHT -1)  // 光照强度行下划线Y坐标
#define RANGE_SOIL_MOISTURE_Y RANGE_TEMPERATURE_Y  // 土壤湿度行的Y坐标（第二页）
#define RANGE_SOIL_MOISTURE_LINT_Y (RANGE_SOIL_MOISTURE_Y + RANGE_LINE_HEIGHT -1)  // 土壤湿度行下划线Y坐标
#define RANGE_RAIN_GAUGE_Y (RANGE_SOIL_MOISTURE_Y + RANGE_LINE_HEIGHT + 5)  // 降雨量行的Y坐标
#define RANGE_RAIN_GAUGE_LINT_Y (RANGE_RAIN_GAUGE_Y + RANGE_LINE_HEIGHT -1)  // 降雨量行下划线Y坐标

/**
 * @brief 渲染阈值设置页界面
 * 
 * 显示所有报警阈值，支持两页显示：
 * - 第一页：温度、湿度、光照强度的阈值
 * - 第二页：土壤湿度、降雨量的阈值
 * 
 * 显示逻辑：
 * - 显示格式：最小值 < 参数名 < 最大值（或 参数名 < 最大值）
 * - 当前选中的阈值项会显示下划线
 * - 编辑模式下，下划线会闪烁（每10次刷新切换一次）
 * - 浏览模式下，下划线固定显示
 * 
 * @note 使用静态变量flashCount实现下划线闪烁效果
 */
void renderRangePage() {
  uint8_t x;
  char msg[50];
  uint8_t msgLength;
  uint8_t underlineX = 0;      // 下划线的X起始坐标
  uint8_t underlineLength = 0;  // 下划线的长度
  uint8_t underLineY = 0;       // 下划线的Y坐标

  // 根据当前编辑索引判断显示第一页还是第二页
  if (rangeEditIndex <= RANGE_EDIT_LIGHT_INTENSITY_MAX) {
    // 第一页：显示温度、湿度、光照强度的阈值
    OLED_PrintString(25, 0, " 报警阈值(1/2) ", &font12x12, OLED_COLOR_REVERSED);
    // 显示温度阈值范围
    int minInt, minDec, maxInt, maxDec;
    floatToIntDec(farmSafeRange.minTemperature, &minInt, &minDec);
    floatToIntDec(farmSafeRange.maxTemperature, &maxInt, &maxDec);
    sprintf(msg, "%d.%d < 温度 < %d.%d", minInt, minDec, maxInt, maxDec);
    msgLength = (strlen(msg) - 2) * 6;  // 计算消息总长度（减去2个中文字符，每个字符6像素宽）
    x = 64 - msgLength / 2;  // 居中显示（64是屏幕中心X坐标）
    OLED_PrintString(x, RANGE_TEMPERATURE_Y, msg, &font12x12, OLED_COLOR_NORMAL);
    
    // 如果当前选中最小温度，计算下划线位置
    if (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MIN) {
      sprintf(msg, "%d.%d", minInt, minDec);
      uint8_t valueLength = strlen(msg);
      underlineX = x;
      underlineLength = valueLength * 6;
      underLineY = RANGE_TEMPERATURE_LINT_Y;
    } 
    // 如果当前选中最大温度，计算下划线位置
    else if (rangeEditIndex == RANGE_EDIT_TEMPERATURE_MAX) {
      sprintf(msg, "%d.%d", maxInt, maxDec);
      uint8_t valueLength = strlen(msg);
      underlineX = x + msgLength - valueLength * 6;  // 最大值在消息末尾
      underlineLength = valueLength * 6;
      underLineY = RANGE_TEMPERATURE_LINT_Y;
    }
  
    // 显示湿度阈值范围
    floatToIntDec(farmSafeRange.minHumidity, &minInt, &minDec);
    floatToIntDec(farmSafeRange.maxHumidity, &maxInt, &maxDec);
    sprintf(msg, "%d.%d < 湿度 < %d.%d", minInt, minDec, maxInt, maxDec);
    msgLength = (strlen(msg) - 2) * 6;
    x = 64 - msgLength / 2;
    OLED_PrintString(x, RANGE_HUMIDITY_Y, msg, &font12x12, OLED_COLOR_NORMAL);
    if (rangeEditIndex == RANGE_EDIT_HUMIDITY_MIN) {
      sprintf(msg, "%d.%d", minInt, minDec);
      uint8_t valueLength = strlen(msg);
      underlineX = x;
      underlineLength = valueLength * 6;
      underLineY = RANGE_HUMIDITY_LINT_Y;
    } else if (rangeEditIndex == RANGE_EDIT_HUMIDITY_MAX) {
        sprintf(msg, "%d.%d", maxInt, maxDec);
      uint8_t valueLength = strlen(msg);
      underlineX = x + msgLength - valueLength * 6;
      underlineLength = valueLength * 6;
      underLineY = RANGE_HUMIDITY_LINT_Y;
    }
  
    // 显示光照强度阈值范围
    sprintf(msg, "%d < 光照 < %d", farmSafeRange.minLightIntensity, farmSafeRange.maxLightIntensity);
    msgLength = (strlen(msg) - 2) * 6;
    x = 64 - msgLength / 2;
    OLED_PrintString(x, RANGE_LIGHT_INTENSITY_Y, msg, &font12x12, OLED_COLOR_NORMAL);
    if (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MIN) {
      sprintf(msg, "%d", farmSafeRange.minLightIntensity);
      uint8_t valueLength = strlen(msg);
      underlineX = x;
      underlineLength = valueLength * 6;
      underLineY = RANGE_LIGHT_INTENSITY_LINT_Y;
    } else if (rangeEditIndex == RANGE_EDIT_LIGHT_INTENSITY_MAX) {
      sprintf(msg, "%d", farmSafeRange.maxLightIntensity);
      uint8_t valueLength = strlen(msg);
      underlineX = x + msgLength - valueLength * 6;
      underlineLength = valueLength * 6;
      underLineY = RANGE_LIGHT_INTENSITY_LINT_Y;
    }
  }else{
    // 第二页：显示土壤湿度、降雨量的阈值
    OLED_PrintString(25, 0, " 报警阈值(2/2) ", &font12x12, OLED_COLOR_REVERSED);
    // 显示土壤湿度阈值范围
    sprintf(msg, "%d < 土壤 < %d", farmSafeRange.minSoilMoisture, farmSafeRange.maxSoilMoisture);
    msgLength = (strlen(msg) - 2) * 6;
    x = 64 - msgLength / 2;
    OLED_PrintString(x, RANGE_SOIL_MOISTURE_Y, msg, &font12x12, OLED_COLOR_NORMAL);
    if (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MIN) {
      sprintf(msg, "%d", farmSafeRange.minSoilMoisture);
      uint8_t valueLength = strlen(msg);
      underlineX = x;
      underlineLength = valueLength * 6;
      underLineY = RANGE_SOIL_MOISTURE_LINT_Y;
    } else if (rangeEditIndex == RANGE_EDIT_SOIL_MOISTURE_MAX) {
      sprintf(msg, "%d", farmSafeRange.maxSoilMoisture);
      uint8_t valueLength = strlen(msg);
      underlineX = x + msgLength - valueLength * 6;
      underlineLength = valueLength * 6;
      underLineY = RANGE_SOIL_MOISTURE_LINT_Y;
    }
    
    // 显示降雨量阈值（只有上限）
    sprintf(msg, "降雨 < %d", farmSafeRange.maxRainGauge);
    msgLength = (strlen(msg) - 2) * 6;
    x = 64 - msgLength / 2;
    OLED_PrintString(x, RANGE_RAIN_GAUGE_Y, msg, &font12x12, OLED_COLOR_NORMAL);
    if (rangeEditIndex == RANGE_EDIT_RAIN_GAUGE_MAX) {
      sprintf(msg, "%d", farmSafeRange.maxRainGauge);
      uint8_t valueLength = strlen(msg);
      underlineX = x + msgLength - valueLength * 6;
      underlineLength = valueLength * 6;
      underLineY = RANGE_RAIN_GAUGE_LINT_Y;
    }
  }
  
  // 绘制下划线（用于指示当前选中的阈值项）
  static uint8_t flashCount = 0;  // 闪烁计数器（静态变量保持状态）
  if (underlineLength > 0) {
    if (rangeEditState == RANGE_EDIT_STATE_EDITING) {
      // 编辑模式：下划线闪烁（每10次刷新切换一次，实现闪烁效果）
      flashCount = (flashCount + 1) % 10;
      if (flashCount < 5) {
        OLED_DrawLine(underlineX, underLineY, underlineX + underlineLength, underLineY, OLED_COLOR_NORMAL);
      } 
    }else{
      // 浏览模式：下划线固定显示
      OLED_DrawLine(underlineX, underLineY, underlineX + underlineLength, underLineY, OLED_COLOR_NORMAL);
    }
  }
}
/**
 * @brief 屏幕显示任务主函数
 * 
 * 任务执行流程：
 * 1. 初始化OLED显示屏
 * 2. 进入主循环：
 *    - 创建新的显示帧缓冲区
 *    - 根据当前页面索引调用相应的渲染函数
 *    - 使用互斥锁保护I2C总线，将帧缓冲区内容发送到OLED显示
 *    - 延时10ms后继续下一次刷新
 * 
 * @param argument 任务参数（未使用）
 * 
 * @note 
 * - 使用i2c1Mutex互斥锁保护I2C1总线，因为OLED和AHT20共享I2C1
 * - 10ms的刷新周期保证显示流畅（100Hz刷新率）
 * - 使用双缓冲机制：先在新缓冲区绘制，再一次性显示，避免闪烁
 */
void StartScreenTask(void *argument) {
  // 初始化OLED显示屏
  OLED_Init();
  
  // 主循环：持续刷新显示内容
  for (;;) {
    // 创建新的显示帧缓冲区（双缓冲机制）
    OLED_NewFrame();
    
    // 根据当前页面索引渲染相应的界面
    switch (pageIndex) {
    case PAGE_HOME:
      renderHomePage();  // 渲染首页：显示环境状态数据
      break;
    case PAGE_RANGE:
      renderRangePage(); // 渲染阈值设置页：显示和编辑报警阈值
      break;
    default:
      break;
    }
    
    // 使用互斥锁保护I2C总线，将帧缓冲区内容发送到OLED显示
    osMutexAcquire(i2c1MutexHandle, osWaitForever);
    OLED_ShowFrame();
    osMutexRelease(i2c1MutexHandle);
    
    // 延时10ms后继续下一次刷新
    osDelay(10);
  }
}
