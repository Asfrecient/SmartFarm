#include "mqtt_client.h"

#include <PubSubClient.h>

static PubSubClient* sClient = nullptr;
static const char* sTopic = "smartfarm/data";

void MqttClient_SetClient(PubSubClient* client) {
  sClient = client;
}

void MqttClient_SetTopic(const char* topic) {
  sTopic = topic;
}

PubSubClient* MqttClient_GetClient() {
  return sClient;
}

bool MqttClient_Publish(const char* payload) {
  if (sClient == nullptr) {
    return false;
  }
  return sClient->publish(sTopic, payload);
}
