#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// 修改为当前可用的 WiFi；iPhone 热点建议打开“最大兼容性”。
static const char* WIFI_SSID = "abc";
static const char* WIFI_PASSWORD = "12345678";
static const char* MQTT_HOST = "broker.emqx.io";
static const uint16_t MQTT_PORT = 1883;
static const char* MQTT_CLIENT_ID = "SmartFarm-ESP32-S3";
static const char* MQTT_TOPIC = "smartfarm/data";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

static HardwareSerial& loraSerial = Serial2;
static String frame;
static bool inFrame = false;
static int braceDepth = 0;

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

static void forwardPayload(const String& payload) {
  if (payload.length() == 0) {
    return;
  }

  Serial.print("收到 LoRa 数据：");
  Serial.println(payload);

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, payload)) {
    Serial.println("JSON 解析失败，已丢弃这条数据。");
    return;
  }

  if (mqttClient.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.print("已发布到 MQTT 主题：");
    Serial.println(MQTT_TOPIC);
  } else {
    Serial.println("MQTT 发布失败，请检查网络或 broker 状态。");
  }
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
  mqttClient.setBufferSize(512);
  Serial.print("MQTT 主题：");
  Serial.println(MQTT_TOPIC);
  frame.reserve(256);
}

void loop() {
  connectWiFi();
  connectMQTT();
  mqttClient.loop();

  while (loraSerial.available()) {
    char ch = (char)loraSerial.read();
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
        forwardPayload(frame);
        resetFrame();
      }
    }
  }

  delay(10);
}
