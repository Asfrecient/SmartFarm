#pragma once

#include <PubSubClient.h>

void MqttClient_SetClient(PubSubClient* client);
void MqttClient_SetTopic(const char* topic);
void MqttClient_SetControlTopic(const char* topic);
void MqttClient_SubscribeControl();
PubSubClient* MqttClient_GetClient();
bool MqttClient_Publish(const char* payload);
