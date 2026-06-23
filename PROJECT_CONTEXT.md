
---

# 课程设计项目背景

课程设计题目：

> 基于LoRa的智慧农业系统设计与实现

目标：

* 在现有 SmartFarm 项目基础上扩展
* 尽量复用现有硬件和代码
* 不推翻重做
* 实现 LoRa 通信
* 实现 MQTT 云端通信
* 实现 Web 监控界面
* 实现 Android Studio 客户端
* 满足课程设计答辩要求

---

# 现有项目

项目来源：

[https://docs.keysking.com/docs/stm32/example/Project_SmartFarm/](https://docs.keysking.com/docs/stm32/example/Project_SmartFarm/)

已拥有：

* SmartFarm全部硬件
* SmartFarm全部源码
* STM32CubeIDE工程

主控：

```text
STM32F103C8T6
```

系统：

```text
FreeRTOS
```

---

# SmartFarm已实现功能

已完成：

* AHT20温湿度采集
* 土壤湿度采集
* 光照采集
* 雨滴检测
* OLED显示
* RGB灯控制
* 蜂鸣器报警
* 自动灌溉（继电器控制）
* LoRa通信

要求：

最大程度复用现有代码。

不要重新设计传感器驱动。

不要重新设计FreeRTOS框架。

---

# 新增硬件

采用：

```text
E32-433T20D ×2
```

工作频率：

```text
433MHz
```

工作模式：

```text
普通透传模式
M0=0
M1=0
```

---

# LoRa节点

SmartFarm开发板：

```text
STM32F103
      ↓
USART3
      ↓
E32-433T20D
```

接线：

```text
PB10(TX) → E32 RXD
PB11(RX) → E32 TXD

5V       → E32 VCC
GND      → E32 GND

GND      → M0
GND      → M1
```

AUX暂不使用。

---

# LoRa网关

采用：

```text
ESP32-S3-N16R8
```

功能：

* LoRa接收
* WiFi联网
* MQTT客户端
* 数据转发

接线：

```text
ESP32-S3      E32

GPIO17(TX) → RXD
GPIO18(RX) ← TXD

3.3V       → VCC
GND        → GND

GND        → M0
GND        → M1
```

AUX暂不使用。

---

# 系统总体架构

```text
AHT20
土壤湿度
光照
雨滴
继电器
蜂鸣器
OLED
     │
     ▼
STM32F103
     │
 USART3
     │
E32-433T20D

~~~~ LoRa ~~~~

E32-433T20D
     │
 UART
     ▼
ESP32-S3
     │
 WiFi
     ▼
MQTT Broker
     │
 ┌─────────────┬─────────────┐
 │ Node-RED Web│ Android App │
 └─────────────┴─────────────┘
```

---

# MQTT方案

采用：

```text
EMQX Public Broker
```

Broker：

```text
broker.emqx.io
```

端口：

```text
1883
```

Topic：

```text
smartfarm/data
```

---

# 数据格式

STM32采集后发送JSON：

```json
{
  "temp": 25.6,
  "hum": 68,
  "soil": 42,
  "light": 520,
  "rain": 0
}
```

LoRa透传。

ESP32接收后：

上传MQTT。

---

# STM32端要求

新增模块：

```text
App/
├── lora_e32.c
├── lora_e32.h
```

提供接口：

```c
void LoRa_Init(void);

void LoRa_Send(const char *data);

void LoRa_Task(void *argument);
```

要求：

* 基于USART3
* FreeRTOS任务方式实现
* 周期发送农业数据
* 不影响现有任务运行

---

# ESP32-S3端要求

开发环境：

```text
Arduino IDE
```

功能：

### WiFi连接

```text
WiFi SSID配置
WiFi自动重连
```

### LoRa数据接收

```text
UART接收E32数据
JSON解析
```

### MQTT上传

```text
连接 broker.emqx.io

发布 Topic：

smartfarm/data
```

建议代码结构：

```text
src/
├── wifi_manager.cpp
├── mqtt_client.cpp
├── lora_gateway.cpp
└── main.cpp
```

---

# Web平台要求

采用：

```text
Node-RED Dashboard
```

显示：

* 温度
* 湿度
* 土壤湿度
* 光照强度
* 降雨状态

要求：

* 实时刷新
* 仪表盘形式
* 课程设计演示效果良好

---

# Android客户端要求

开发工具：

```text
Android Studio
```

运行环境：

```text
Android Emulator
```

不依赖真机。

---

# Android通信方式

采用：

```text
MQTT
```

连接：

```text
tcp://broker.emqx.io:1883
```

订阅：

```text
smartfarm/data
```

---

# Android功能要求

首页实时显示：

```text
温度
湿度
土壤湿度
光照强度
降雨状态
```

自动刷新。

---

# 历史数据页面

使用：

```text
MPAndroidChart
```

显示：

* 温度曲线
* 湿度曲线
* 土壤湿度曲线

保存最近50条数据即可。

---

# 系统状态页面

显示：

```text
MQTT连接状态

LoRa状态

ESP32网关状态

最后更新时间
```

---

# Android工程结构

```text
app/
├── ui/
│   ├── MainActivity
│   ├── DashboardFragment
│   ├── HistoryFragment
│   └── StatusFragment
│
├── mqtt/
│   ├── MQTTManager
│   └── MQTTConfig
│
├── model/
│   └── SensorData
│
└── chart/
    └── ChartHelper
```

---

# 数据库设计

课程设计阶段：

可选实现。

如果实现：

```text
MySQL
```

存储：

* 温度
* 湿度
* 土壤湿度
* 光照
* 降雨状态
* 时间戳

---

# 课程设计最终输出

请基于现有 SmartFarm 工程进行改造。

输出：

1. 系统总体设计
2. 硬件设计
3. 软件设计
4. FreeRTOS任务设计
5. LoRa通信设计
6. MQTT设计
7. Node-RED设计
8. Android设计
9. 数据库设计
10. 系统测试方案
11. 系统架构图
12. 数据流图
13. 模块关系图
14. STM32代码实现
15. ESP32代码实现
16. Android代码实现
17. 完整课程设计报告

重点：

* 最大程度复用SmartFarm现有代码
* 不重新设计传感器部分
* LoRa采用E32透传方案
* Android必须基于Android Studio开发并在模拟器中演示
* 整体方案应符合本科《基于LoRa的智慧农业系统》课程设计要求。
