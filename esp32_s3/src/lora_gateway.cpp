#include "lora_gateway.h"

#include "mqtt_client.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>

static HardwareSerial* sSerial = nullptr;
static String sFrame;
static bool sInFrame = false;
static int sBraceDepth = 0;
static String sPendingCommand;
static uint8_t sPendingCommandRepeats = 0;
static unsigned long sLastRxMillis = 0;
static unsigned long sLastCommandMillis = 0;
static unsigned long sNextCommandSendMillis = 0;
static bool sDownlinkWindowOpen = false;
static unsigned long sDownlinkWindowUntilMillis = 0;
static bool sCommandSentInCurrentWindow = false;
static int sPendingPumpTarget = -2;

static String normalizePumpCommand(const char* payload) {
  String normalized = payload;
  normalized.trim();
  if (normalized == "P1") {
    return "{\"pump\":1}";
  }
  if (normalized == "P0") {
    return "{\"pump\":0}";
  }
  if (normalized == "PA") {
    return "{\"pump\":\"auto\"}";
  }

  StaticJsonDocument<96> doc;
  if (deserializeJson(doc, normalized)) {
    return "";
  }
  JsonVariant pump = doc["pump"];
  if (pump.is<int>()) {
    int value = pump.as<int>();
    if (value > 0) {
      return "{\"pump\":1}";
    }
    return "{\"pump\":0}";
  }
  if (pump.is<const char*>() && String(pump.as<const char*>()) == "auto") {
    return "{\"pump\":\"auto\"}";
  }
  return "";
}

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

static void sendPendingCommand() {
  if (sSerial == nullptr || sPendingCommand.length() == 0 || sPendingCommandRepeats == 0) {
    return;
  }

  unsigned long now = millis();
  if (!sDownlinkWindowOpen) {
    return;
  }
  if (now > sDownlinkWindowUntilMillis) {
    return;
  }
  if (sCommandSentInCurrentWindow || now < sNextCommandSendMillis || now - sLastRxMillis < 300UL) {
    return;
  }

  sSerial->print(sPendingCommand);
  sSerial->print("\r\n");
  sSerial->flush();
  sPendingCommandRepeats--;
  sLastCommandMillis = now;
  sCommandSentInCurrentWindow = true;
  if (sPendingCommandRepeats == 0) {
    sPendingCommand = "";
    sDownlinkWindowOpen = false;
    sDownlinkWindowUntilMillis = 0;
    sNextCommandSendMillis = 0;
    sCommandSentInCurrentWindow = false;
    sPendingPumpTarget = -2;
  }
}

static bool forwardJson(PubSubClient& client, const String& json) {
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, json)) {
    return false;
  }
  if (client.connected()) {
    MqttClient_Publish(json.c_str());
    client.loop();
  }
  JsonVariant pumpValue = doc["pump"];
  JsonVariant pumpManualValue = doc["pumpManual"];
  if (sPendingPumpTarget != -2) {
    bool acknowledged = false;
    if (sPendingPumpTarget == -1) {
      acknowledged = pumpManualValue.is<int>() && pumpManualValue.as<int>() == 0;
    } else if (pumpValue.is<int>() && pumpManualValue.is<int>()) {
      acknowledged = pumpValue.as<int>() == sPendingPumpTarget && pumpManualValue.as<int>() == 1;
    }
    if (acknowledged) {
      sPendingCommand = "";
      sPendingCommandRepeats = 0;
      sPendingPumpTarget = -2;
      sDownlinkWindowOpen = false;
      sDownlinkWindowUntilMillis = 0;
      sNextCommandSendMillis = 0;
      sCommandSentInCurrentWindow = false;
    }
  }
  return true;
}

void LoraGateway_Loop(PubSubClient& client) {
  while (sSerial && sSerial->available()) {
    char ch = (char)sSerial->read();
    sLastRxMillis = millis();
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
        bool validFrame = forwardJson(client, sFrame);
        resetFrame();
        if (validFrame && sPendingCommand.length() > 0 && sPendingCommandRepeats > 0) {
          unsigned long now = millis();
          sDownlinkWindowOpen = true;
          sNextCommandSendMillis = now + 800UL;
          sDownlinkWindowUntilMillis = now + 1800UL;
          sCommandSentInCurrentWindow = false;
          sendPendingCommand();
        }
      }
    }
  }

  sendPendingCommand();
}

bool LoraGateway_SendCommand(const char* payload) {
  if (sSerial == nullptr || payload == nullptr || payload[0] == '\0') {
    return false;
  }
  String command = normalizePumpCommand(payload);
  if (command.length() == 0) {
    return false;
  }
  sPendingCommand = command;
  if (command.indexOf("\"pump\":1") >= 0) {
    sPendingPumpTarget = 1;
  } else if (command.indexOf("\"pump\":0") >= 0) {
    sPendingPumpTarget = 0;
  } else {
    sPendingPumpTarget = -1;
  }
  sPendingCommandRepeats = 8;
  sLastCommandMillis = 0;
  sNextCommandSendMillis = 0;
  sDownlinkWindowOpen = false;
  sDownlinkWindowUntilMillis = 0;
  sCommandSentInCurrentWindow = false;
  return true;
}

bool LoraGateway_HasPendingCommand() {
  return sPendingCommandRepeats > 0;
}
