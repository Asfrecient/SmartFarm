#include "lora_gateway.h"

#include "mqtt_client.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>

static HardwareSerial* sSerial = nullptr;
static String sFrame;
static bool sInFrame = false;
static int sBraceDepth = 0;

void LoraGateway_Init(HardwareSerial& serial, int txPin, int rxPin, long baudRate) {
  sSerial = &serial;
  sSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
  sFrame.reserve(256);
}

static void resetFrame() {
  sFrame = "";
  sInFrame = false;
  sBraceDepth = 0;
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
    if (!sInFrame) {
      if (ch == '{') {
        sInFrame = true;
        sBraceDepth = 1;
        sFrame = "{";
      }
      continue;
    }

    if (sFrame.length() > 240) {
      resetFrame();
      continue;
    }

    sFrame += ch;
    if (ch == '{') {
      sBraceDepth++;
    } else if (ch == '}') {
      sBraceDepth--;
      if (sBraceDepth == 0) {
        forwardJson(client, sFrame);
        resetFrame();
      }
    }
  }
}
