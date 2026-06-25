#include "lora_e32.h"

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "global/farmState.h"
#include "main.h"
#include "pump/pump.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include "task.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LORA_RX_BUFFER_SIZE 128U
#define LORA_REPORT_INTERVAL_MS 3000U
#define LORA_PUMP_SELF_TEST_ON_BOOT 0

static uint8_t loraRxByte = 0;
static volatile uint8_t loraRxBuffer[LORA_RX_BUFFER_SIZE];
static volatile uint16_t loraRxHead = 0;
static volatile uint16_t loraRxTail = 0;
static volatile uint32_t loraRxByteCount = 0;
static volatile uint32_t loraPumpCommandCount = 0;
static volatile uint32_t loraRxStartOkCount = 0;
static volatile uint32_t loraRxStartBusyCount = 0;
static volatile uint32_t loraRxStartErrorCount = 0;
static volatile uint32_t loraUartErrorCount = 0;
static volatile uint32_t loraLastUartError = 0;

static void LoRa_ApplyPumpCommand(uint8_t pumpState, uint8_t manualMode);

static uint8_t LoRa_CheckAlarm(const FarmState *state) {
  if (state->temperature < farmSafeRange.minTemperature || state->temperature > farmSafeRange.maxTemperature) return 1;
  if (state->humidity < farmSafeRange.minHumidity || state->humidity > farmSafeRange.maxHumidity) return 1;
  if (state->rainGauge > farmSafeRange.maxRainGauge) return 1;
  if (state->soilMoisture < farmSafeRange.minSoilMoisture || state->soilMoisture > farmSafeRange.maxSoilMoisture) return 1;
  if (state->lightIntensity < farmSafeRange.minLightIntensity || state->lightIntensity > farmSafeRange.maxLightIntensity) return 1;
  return 0;
}

static void LoRa_RestartReceive(void) {
  HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart3, &loraRxByte, 1);
  if (status == HAL_OK) {
    loraRxStartOkCount++;
  } else if (status == HAL_BUSY) {
    loraRxStartBusyCount++;
  } else {
    loraRxStartErrorCount++;
  }
}

static void LoRa_StoreRxByte(uint8_t ch) {
  uint16_t next = (uint16_t)((loraRxHead + 1U) % LORA_RX_BUFFER_SIZE);
  if (next == loraRxTail) {
    return;
  }
  loraRxBuffer[loraRxHead] = ch;
  loraRxHead = next;
  loraRxByteCount++;
}

static uint8_t LoRa_ReadByte(uint8_t *ch) {
  if (ch == NULL) {
    return 0;
  }

  taskENTER_CRITICAL();
  if (loraRxHead == loraRxTail) {
    taskEXIT_CRITICAL();
    return 0;
  }
  *ch = loraRxBuffer[loraRxTail];
  loraRxTail = (uint16_t)((loraRxTail + 1U) % LORA_RX_BUFFER_SIZE);
  taskEXIT_CRITICAL();
  return 1;
}

static void LoRa_ApplyPumpCommand(uint8_t pumpState, uint8_t manualMode) {
  if (manualMode > 0) {
    if (pumpState > 0) {
      Pump_On();
    } else {
      Pump_Off();
    }
  }

  taskENTER_CRITICAL();
  farmState.waterPumpManualMode = manualMode;
  if (manualMode > 0) {
    farmState.waterPumpState = pumpState;
  }
  loraPumpCommandCount++;
  taskEXIT_CRITICAL();
}

static uint8_t LoRa_ProcessJsonCommand(const char *frame) {
  if (frame == NULL) {
    return 0;
  }

  if (strstr(frame, "\"pump\":\"auto\"") != NULL || strstr(frame, "\"pump\": \"auto\"") != NULL) {
    LoRa_ApplyPumpCommand(farmState.waterPumpState, 0);
    return 1;
  }

  if (strstr(frame, "\"pump\":1") != NULL || strstr(frame, "\"pump\": 1") != NULL) {
    LoRa_ApplyPumpCommand(1, 1);
    return 1;
  }

  if (strstr(frame, "\"pump\":0") != NULL || strstr(frame, "\"pump\": 0") != NULL) {
    LoRa_ApplyPumpCommand(0, 1);
    return 1;
  }

  return 0;
}

static void LoRa_ProcessIncoming(void) {
  static char frame[96];
  static uint8_t inFrame = 0;
  static uint8_t depth = 0;
  static uint8_t index = 0;
  static uint8_t lastCh = 0;
  uint8_t ch = 0;

  while (LoRa_ReadByte(&ch)) {
    if (lastCh == 'P') {
      if (ch == '1') {
        LoRa_ApplyPumpCommand(1, 1);
      } else if (ch == '0') {
        LoRa_ApplyPumpCommand(0, 1);
      } else if (ch == 'A' || ch == 'a') {
        LoRa_ApplyPumpCommand(farmState.waterPumpState, 0);
      }
    }
    lastCh = ch;

    if (!inFrame) {
      if (ch == '{') {
        inFrame = 1;
        depth = 1;
        index = 0;
        frame[index++] = (char)ch;
      }
      continue;
    }

    if (index >= sizeof(frame) - 1) {
      inFrame = 0;
      depth = 0;
      index = 0;
      continue;
    }

    frame[index++] = (char)ch;
    if (ch == '{') {
      depth++;
    } else if (ch == '}') {
      depth--;
      if (depth == 0) {
        frame[index] = '\0';
        LoRa_ProcessJsonCommand(frame);
        inFrame = 0;
        index = 0;
      }
    }
  }
}

void LoRa_Init(void) {
  /* E32-433T20D 普通透传模式下无需额外配置。
   * 预留统一初始化入口，后续若增加AT配置可直接扩展。 */
  __HAL_UART_CLEAR_OREFLAG(&huart3);
  __HAL_UART_CLEAR_FEFLAG(&huart3);
  __HAL_UART_CLEAR_NEFLAG(&huart3);
  LoRa_RestartReceive();
}

void LoRa_Send(const char *data) {
  if (data == NULL) {
    return;
  }
  HAL_UART_Transmit(&huart3, (uint8_t *)data, (uint16_t)strlen(data), HAL_MAX_DELAY);
  LoRa_RestartReceive();
}

void LoRa_Task(void *argument) {
  (void)argument;
  LoRa_Init();
  uint32_t lastReportTick = 0;

#if LORA_PUMP_SELF_TEST_ON_BOOT
  Pump_On();
  osDelay(1500);
  Pump_Off();
#endif

  for (;;) {
    LoRa_ProcessIncoming();

    uint32_t now = osKernelGetTickCount();
    if (now - lastReportTick < LORA_REPORT_INTERVAL_MS) {
      osDelay(20);
      continue;
    }
    lastReportTick = now;

    FarmState snapshot;
    uint32_t rxByteCount = 0;
    uint32_t pumpCommandCount = 0;

    taskENTER_CRITICAL();
    snapshot = farmState;
    rxByteCount = loraRxByteCount;
    pumpCommandCount = loraPumpCommandCount;
    taskEXIT_CRITICAL();

    char payload[256];
    uint8_t alarm = LoRa_CheckAlarm(&snapshot);
    int tempInt = 0;
    int tempDec = 0;
    int humInt = 0;
    int humDec = 0;
    floatToIntDec(snapshot.temperature, &tempInt, &tempDec);
    floatToIntDec(snapshot.humidity, &humInt, &humDec);

    snprintf(payload, sizeof(payload),
             "{\"temp\":%d.%d,\"hum\":%d.%d,\"soil\":%u,\"light\":%u,\"rain\":%u,\"pump\":%u,\"pumpManual\":%u,\"alarm\":%u,\"rx\":%lu,\"cmd\":%lu}\r\n",
             tempInt,
             tempDec,
             humInt,
             humDec,
             snapshot.soilMoisture,
             snapshot.lightIntensity,
             snapshot.rainGauge,
             snapshot.waterPumpState,
             snapshot.waterPumpManualMode,
             alarm,
             rxByteCount,
             pumpCommandCount);
    LoRa_Send(payload);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart3) {
    LoRa_StoreRxByte(loraRxByte);
    LoRa_RestartReceive();
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart == &huart3) {
    loraLastUartError = HAL_UART_GetError(huart);
    loraUartErrorCount++;
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    LoRa_RestartReceive();
  }
}
