#include "lora_gateway.h"

#include "mqtt_client.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>

static HardwareSerial* sSerial = nullptr;
static String sBuffer;

void LoraGateway_Init(HardwareSerial& serial, int txPin, int rxPin, long baudRate) {
  sSerial = &serial;
  sSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
  sBuffer.reserve(256);
}

static void forwardJson(PubSubClient& client, const String& json) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, json)) {
    return;
  }
  if (client.connected()) {
    MqttClient_Publish(json.c_str());
    client.loop();
  }
}

void LoraGateway_Loop(PubSubClient& client) {
  while (sSerial && sSerial->available()) {
    char ch = (char)sSerial->read();
    if (ch == '\n') {
      forwardJson(client, sBuffer);
      sBuffer = "";
    } else if (ch != '\r') {
      sBuffer += ch;
      if (sBuffer.length() > 240) {
        sBuffer = "";
      }
    }
  }
}
