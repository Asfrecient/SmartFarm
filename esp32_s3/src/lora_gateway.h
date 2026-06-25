#pragma once

#include <PubSubClient.h>
#include <HardwareSerial.h>

void LoraGateway_Init(HardwareSerial& serial, int txPin, int rxPin, long baudRate);
void LoraGateway_Loop(PubSubClient& client);
bool LoraGateway_SendCommand(const char* payload);
bool LoraGateway_HasPendingCommand();
