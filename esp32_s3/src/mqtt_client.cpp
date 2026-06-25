#include "mqtt_client.h"

#include "lora_gateway.h"

#include <PubSubClient.h>
#include <string.h>

static PubSubClient* sClient = nullptr;
static const char* sTopic = "smartfarm/data";
static const char* sControlTopic = "smartfarm/control";

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (topic == nullptr || strcmp(topic, sControlTopic) != 0 || length == 0) {
    return;
  }

  char command[96];
  unsigned int copyLength = length < sizeof(command) - 1 ? length : sizeof(command) - 1;
  memcpy(command, payload, copyLength);
  command[copyLength] = '\0';
  LoraGateway_SendCommand(command);
}

void MqttClient_SetClient(PubSubClient* client) {
  sClient = client;
  if (sClient != nullptr) {
    sClient->setCallback(onMqttMessage);
  }
}

void MqttClient_SetTopic(const char* topic) {
  sTopic = topic;
}

void MqttClient_SetControlTopic(const char* topic) {
  sControlTopic = topic;
}

void MqttClient_SubscribeControl() {
  if (sClient != nullptr && sClient->connected()) {
    sClient->subscribe(sControlTopic, 0);
  }
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
