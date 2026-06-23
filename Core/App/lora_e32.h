#ifndef SMARTFARM_LORA_E32_H
#define SMARTFARM_LORA_E32_H

#include "cmsis_os2.h"

void LoRa_Init(void);
void LoRa_Send(const char *data);
void LoRa_Task(void *argument);

#endif
