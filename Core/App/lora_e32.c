#include "lora_e32.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "global/farmState.h"
#include "main.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include "task.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t LoRa_CheckAlarm(const FarmState *state) {
  if (state->temperature < farmSafeRange.minTemperature || state->temperature > farmSafeRange.maxTemperature) return 1;
  if (state->humidity < farmSafeRange.minHumidity || state->humidity > farmSafeRange.maxHumidity) return 1;
  if (state->rainGauge > farmSafeRange.maxRainGauge) return 1;
  if (state->soilMoisture < farmSafeRange.minSoilMoisture || state->soilMoisture > farmSafeRange.maxSoilMoisture) return 1;
  if (state->lightIntensity < farmSafeRange.minLightIntensity || state->lightIntensity > farmSafeRange.maxLightIntensity) return 1;
  return 0;
}

void LoRa_Init(void) {
  /* E32-433T20D 普通透传模式下无需额外配置。
   * 预留统一初始化入口，后续若增加AT配置可直接扩展。 */
}

void LoRa_Send(const char *data) {
  if (data == NULL) {
    return;
  }
  HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)strlen(data), HAL_MAX_DELAY);
}

void LoRa_Task(void *argument) {
  (void)argument;
  LoRa_Init();

  for (;;) {
    FarmState snapshot;

    taskENTER_CRITICAL();
    snapshot = farmState;
    taskEXIT_CRITICAL();

    char payload[192];
    uint8_t alarm = LoRa_CheckAlarm(&snapshot);
    int tempInt = 0;
    int tempDec = 0;
    int humInt = 0;
    int humDec = 0;
    floatToIntDec(snapshot.temperature, &tempInt, &tempDec);
    floatToIntDec(snapshot.humidity, &humInt, &humDec);

    snprintf(payload, sizeof(payload),
             "{\"temp\":%d.%d,\"hum\":%d.%d,\"soil\":%u,\"light\":%u,\"rain\":%u,\"pump\":%u,\"alarm\":%u}\r\n",
             tempInt,
             tempDec,
             humInt,
             humDec,
             snapshot.soilMoisture,
             snapshot.lightIntensity,
             snapshot.rainGauge,
             snapshot.waterPumpState,
             alarm);
    LoRa_Send(payload);
    osDelay(1000);
  }
}
