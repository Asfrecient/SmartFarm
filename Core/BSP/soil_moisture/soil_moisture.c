//
// Created by keysk on 2025/11/20.
//

#include "soil_moisture.h"
#include "adc.h"

void SoilMoisture_init(void) {
  HAL_ADC_Start(&hadc2);
}

/**
 * 获得土壤湿度值
 * 此值仅是土壤湿度大时导致传感器电阻减小
 * 随后测量到的传感器的电压值
 * 仅能相对地展示土壤湿度的大小与变化
 * @return 0~100的土壤湿度值
 */
uint16_t SoilMoisture_Get() {
  uint16_t adc = HAL_ADC_GetValue(&hadc2);
  if (adc > 4000) {
    adc = 4000;
  }
  return 100 - adc / 40;
}
