#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 修改为当前可用的 WiFi；iPhone 热点建议打开“最大兼容性”。
static const char* WIFI_SSID = "abc";
static const char* WIFI_PASSWORD = "12345678";
static const char* MQTT_HOST = "broker.emqx.io";
static const uint16_t MQTT_PORT = 1883;
static const char* MQTT_CLIENT_ID = "SmartFarm-ESP32-S3";
static const char* MQTT_DATA_TOPIC = "smartfarm/data";
static const char* MQTT_CONTROL_TOPIC = "smartfarm/control";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static HardwareSerial& loraSerial = Serial2;
static String frame;
static bool inFrame = false;
static int braceDepth = 0;
static String pendingCommand;
static uint8_t pendingCommandRepeats = 0;
static unsigned long lastLoraRxMillis = 0;
static unsigned long lastCommandSendMillis = 0;
static unsigned long nextCommandSendMillis = 0;
static unsigned long downlinkWindowUntilMillis = 0;
static bool commandSentInCurrentWindow = false;
static int pendingPumpTarget = -2;
static String serialCommandBuffer;

static String normalizePumpCommand(const String& command) {
  String normalized = command;
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

static void queuePumpCommand(const String& command) {
  if (command.length() == 0) {
    return;
  }
  pendingCommand = command;
  if (command.indexOf("\"pump\":1") >= 0) {
    pendingPumpTarget = 1;
  } else if (command.indexOf("\"pump\":0") >= 0) {
    pendingPumpTarget = 0;
  } else {
    pendingPumpTarget = -1;
  }
  pendingCommandRepeats = 8;
  lastCommandSendMillis = 0;
  nextCommandSendMillis = 0;
  downlinkWindowUntilMillis = 0;
  commandSentInCurrentWindow = false;
  Serial.print("控制命令已缓存：");
  Serial.println(pendingCommand);
  Serial.println("等待下一帧 LoRa 上报结束后下发控制命令。");
}

static void pollUsbSerialCommand() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r' || ch == '\n') {
      if (serialCommandBuffer.length() == 0) {
        continue;
      }
      serialCommandBuffer.trim();
      Serial.print("收到 USB 调试命令：");
      Serial.println(serialCommandBuffer);
      String command = normalizePumpCommand(serialCommandBuffer);
      if (command.length() == 0) {
        Serial.println("USB 调试命令无效，支持 P1 / P0 / PA 或 {\"pump\":1} 这类 JSON。");
      } else {
        queuePumpCommand(command);
      }
      serialCommandBuffer = "";
      continue;
    }
    serialCommandBuffer += ch;
    if (serialCommandBuffer.length() > 64) {
      serialCommandBuffer = "";
      Serial.println("USB 调试命令过长，已清空缓冲。");
    }
  }
}

static void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("正在连接 WiFi：");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t retryCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retryCount++;
    if (retryCount % 20 == 0) {
      Serial.println();
      Serial.println("WiFi 还没有连接成功，请检查热点是否开启、名称密码是否正确。");
      Serial.print("当前 WiFi 状态码：");
      Serial.println(WiFi.status());
    }
  }

  Serial.println();
  Serial.println("WiFi 已连接。");
  Serial.print("ESP32-S3 IP 地址：");
  Serial.println(WiFi.localIP());
}

static void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("正在连接 MQTT：");
    Serial.print(MQTT_HOST);
    Serial.print(":");
    Serial.println(MQTT_PORT);
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT 已连接。");
      mqttClient.subscribe(MQTT_CONTROL_TOPIC, 0);
      Serial.print("已订阅控制主题：");
      Serial.println(MQTT_CONTROL_TOPIC);
      break;
    }
    if (!mqttClient.connected()) {
      Serial.print("MQTT 连接失败，状态码：");
      Serial.println(mqttClient.state());
      Serial.println("1 秒后重试。");
      delay(1000);
    }
  }
}

static void sendPendingCommand() {
  if (pendingCommand.length() == 0 || pendingCommandRepeats == 0) {
    return;
  }

  unsigned long now = millis();
  if (now > downlinkWindowUntilMillis) {
    return;
  }
  if (commandSentInCurrentWindow || now < nextCommandSendMillis || now - lastLoraRxMillis < 300UL) {
    return;
  }

  Serial.print("LoRa 下发控制命令：");
  Serial.println(pendingCommand);
  loraSerial.print(pendingCommand);
  loraSerial.print("\r\n");
  loraSerial.flush();
  pendingCommandRepeats--;
  lastCommandSendMillis = now;
  commandSentInCurrentWindow = true;
  if (pendingCommandRepeats == 0) {
    pendingCommand = "";
    downlinkWindowUntilMillis = 0;
    nextCommandSendMillis = 0;
    commandSentInCurrentWindow = false;
    pendingPumpTarget = -2;
  }
}

static bool forwardPayload(const String& payload) {
  if (payload.length() == 0) {
    return false;
  }

  Serial.print("收到 LoRa 数据：");
  Serial.println(payload);

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, payload)) {
    Serial.println("JSON 解析失败，已丢弃这条数据。");
    return false;
  }

  if (mqttClient.publish(MQTT_DATA_TOPIC, payload.c_str())) {
    Serial.print("已发布到 MQTT 主题：");
    Serial.println(MQTT_DATA_TOPIC);
  } else {
    Serial.println("MQTT 发布失败，请检查网络或 broker 状态。");
  }

  JsonVariant pumpValue = doc["pump"];
  JsonVariant pumpManualValue = doc["pumpManual"];
  if (pendingPumpTarget != -2) {
    bool acknowledged = false;
    if (pendingPumpTarget == -1) {
      acknowledged = pumpManualValue.is<int>() && pumpManualValue.as<int>() == 0;
    } else if (pumpValue.is<int>() && pumpManualValue.is<int>()) {
      acknowledged = pumpValue.as<int>() == pendingPumpTarget && pumpManualValue.as<int>() == 1;
    }
    if (acknowledged) {
      Serial.print("控制命令已收到状态确认，停止重试：");
      Serial.println(pendingCommand);
      pendingCommand = "";
      pendingCommandRepeats = 0;
      pendingPumpTarget = -2;
      downlinkWindowUntilMillis = 0;
      nextCommandSendMillis = 0;
      commandSentInCurrentWindow = false;
    }
  }
  return true;
}

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (topic == nullptr || String(topic) != MQTT_CONTROL_TOPIC || length == 0) {
    return;
  }

  String command;
  command.reserve(length + 2);
  for (unsigned int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  Serial.print("收到 MQTT 控制命令：");
  Serial.println(command);
  String normalizedCommand = normalizePumpCommand(command);
  if (normalizedCommand.length() == 0) {
    Serial.println("控制命令格式无效，已忽略。");
    return;
  }
  Serial.print("控制命令已规范化为 LoRa 下发 JSON：");
  Serial.println(normalizedCommand);
  queuePumpCommand(normalizedCommand);
}

static void resetFrame() {
  frame = "";
  inFrame = false;
  braceDepth = 0;
}

void setup() {
  Serial.begin(115200);
  unsigned long serialStartMillis = millis();
  while (!Serial && millis() - serialStartMillis < 5000) {
    delay(10);
  }
  delay(500);
  Serial.println();
  Serial.println("智慧农场 ESP32-S3 网关启动。");
  Serial.println("串口监视器波特率：115200");

  loraSerial.begin(9600, SERIAL_8N1, 18, 17);
  Serial.println("LoRa 串口已启动：Serial2, RX=GPIO18, TX=GPIO17, baud=9600");

  WiFi.mode(WIFI_STA);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);
  Serial.print("MQTT 数据主题：");
  Serial.println(MQTT_DATA_TOPIC);
  Serial.print("MQTT 控制主题：");
  Serial.println(MQTT_CONTROL_TOPIC);
  frame.reserve(256);
}

void loop() {
  connectWiFi();
  connectMQTT();
  mqttClient.loop();
  pollUsbSerialCommand();

  while (loraSerial.available()) {
    char ch = (char)loraSerial.read();
    lastLoraRxMillis = millis();
    if (!inFrame) {
      if (ch == '{') {
        inFrame = true;
        braceDepth = 1;
        frame = "{";
      }
      continue;
    }

    if (frame.length() > 240) {
      Serial.println("LoRa 帧超过 240 字节，已丢弃。");
      resetFrame();
      continue;
    }

    frame += ch;
    if (ch == '{') {
      braceDepth++;
    } else if (ch == '}') {
      braceDepth--;
      if (braceDepth == 0) {
        bool validFrame = forwardPayload(frame);
        resetFrame();
        if (validFrame && pendingCommand.length() > 0 && pendingCommandRepeats > 0) {
          unsigned long now = millis();
          nextCommandSendMillis = now + 800UL;
          downlinkWindowUntilMillis = now + 1800UL;
          commandSentInCurrentWindow = false;
          sendPendingCommand();
        }
      }
    }
  }

  sendPendingCommand();
  delay(10);
}
