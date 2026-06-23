#pragma once

#include <PubSubClient.h>

void WifiManager_Init(PubSubClient& client, const char* host, uint16_t port, const char* clientId);
void WifiManager_Loop(PubSubClient& client);
