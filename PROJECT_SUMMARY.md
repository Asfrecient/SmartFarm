# SmartFarm 项目总结

## 项目定位

SmartFarm 是一个基于 STM32F103C8T6 和 FreeRTOS 的智慧农业监控系统课程设计项目。项目在原有 STM32 智能农场例程基础上扩展 LoRa、ESP32-S3、MQTT、Node-RED Web 仪表盘和 Android 客户端，形成从现场采集到云端展示的完整物联网链路。

## 系统架构

```text
传感器/执行器
  AHT20 温湿度、土壤湿度、光照、雨滴、继电器、蜂鸣器、OLED
        |
        v
STM32F103 + FreeRTOS
        |
        v
E32-433T20D LoRa
        |
        v
ESP32-S3 网关
        |
        v
MQTT Broker
        |
        +-- Node-RED Web Dashboard
        +-- Android 客户端
```

## 主要功能

- STM32 端周期采集温湿度、土壤湿度、光照和雨滴数据。
- OLED 显示实时环境数据和阈值设置页面。
- 按键和旋钮支持页面切换、阈值选择和阈值编辑。
- FreeRTOS 任务拆分传感器采集、输入处理、屏幕刷新和 LoRa 发送。
- 蜂鸣器和继电器用于异常报警与灌溉控制。
- STM32 通过 USART3 向 E32-433T20D 发送 JSON 数据帧。
- ESP32-S3 网关接收 LoRa 数据后连接 WiFi，并发布到 MQTT 主题 `smartfarm/data`。
- Node-RED Dashboard 和 Android 客户端订阅 MQTT 数据，实现远程监控展示。

## 代码结构

- `Core/`：STM32 主工程代码，包含 HAL 初始化、FreeRTOS 配置、应用任务和 BSP 驱动。
- `Core/App/`：传感器任务、输入任务、显示任务、LoRa 通信和全局状态管理。
- `Core/BSP/`：OLED、AHT20、按键、旋钮、光照、雨滴、土壤湿度、水泵等硬件驱动。
- `esp32_s3/SmartFarmGateway/`：ESP32-S3 Arduino 网关工程。
- `android_smartfarm/`：Android Studio 客户端工程。
- `web/`：Node-RED Dashboard 流程文件。
- `docs/hardware/`：硬件方案、架构、约束、验证和决策文档。
- `联调步骤.md`：STM32、ESP32、Android 和 Web 端联调流程。

## 通信数据格式

STM32 每秒通过 LoRa 发送一帧 JSON：

```json
{"temp":25.6,"hum":68.0,"soil":42,"light":520,"rain":0,"pump":1,"alarm":0}
```

ESP32-S3 将收到的数据发布到：

```text
broker.emqx.io:1883
topic: smartfarm/data
```

## 项目成果

本项目完成了从嵌入式采集、LoRa 无线传输、ESP32 网关转发、MQTT 云端通信到 Web/Android 可视化展示的课程设计闭环。系统保留原 SmartFarm 教学例程的 FreeRTOS 多任务结构，同时补齐了物联网远程监控链路，适合用于课程答辩、演示和后续扩展。
