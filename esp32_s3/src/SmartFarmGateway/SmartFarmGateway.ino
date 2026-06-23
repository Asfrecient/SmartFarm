#include <WiFi.h>
#include <PubSubClient.h>

#include "wifi_manager.h"
#include "mqtt_client.h"
#include "lora_gateway.h"

static const char* kBrokerHost = "broker.emqx.io";
static const uint16_t kBrokerPort = 1883;
static const char* kTopic = "smartfarm/data";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup() {
  Serial.begin(115200);
  LoraGateway_Init(Serial2, 17, 18, 9600);
  WifiManager_Init(mqttClient, kBrokerHost, kBrokerPort, "SmartFarm-ESP32-S3");
  MqttClient_SetTopic(kTopic);
  mqttClient.setBufferSize(512);
}

void loop() {
  WifiManager_Loop(mqttClient);
  LoraGateway_Loop(mqttClient);
  delay(10);
}
