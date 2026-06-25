#include "wifi_manager.h"

#include "mqtt_client.h"

#include <WiFi.h>

static const char* kSsid = "请输入你的WiFi名称";
static const char* kPassword = "请输入你的WiFi密码";
static const char* kHost = nullptr;
static uint16_t kPort = 1883;
static const char* kClientId = nullptr;

void WifiManager_Init(PubSubClient& client, const char* host, uint16_t port, const char* clientId) {
  kHost = host;
  kPort = port;
  kClientId = clientId;
  WiFi.mode(WIFI_STA);
  WiFi.begin(kSsid, kPassword);
  client.setServer(kHost, kPort);
  MqttClient_SetClient(&client);
}

static void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  WiFi.disconnect(false);
  WiFi.begin(kSsid, kPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void WifiManager_Loop(PubSubClient& client) {
  ensureWiFi();
  if (client.connected()) {
    client.loop();
    return;
  }
  if (client.connect(kClientId)) {
    MqttClient_SubscribeControl();
    client.loop();
  }
}
