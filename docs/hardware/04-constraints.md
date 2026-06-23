# SmartFarm 课程设计报告
# 阶段 5：约束输出
# 日期：2026-06-23

## 系统框图

STM32 采集与控制 → E32-433T20D → ESP32-S3 网关 → MQTT Broker → Node-RED / Android

## 接口矩阵

| 信号 | 方向 | 电平/协议 | 备注 |
|---|---|---|---|
| PB10/TX3 | STM32 -> E32 | UART3 | JSON 发送 |
| PB11/RX3 | E32 -> STM32 | UART3 | 预留配置扩展 |
| GPIO17/TX | ESP32 -> E32 | UART | 网关侧发送 |
| GPIO18/RX | E32 -> ESP32 | UART | 网关侧接收 |
| WiFi | ESP32 -> 云端 | TCP/MQTT | broker.emqx.io |
| MQTT | 云端 -> Web/Android | Topic | smartfarm/data |

## 电源树

| 电源轨 | 负载 |
|---|---|
| 5V | E32 模块（按模块规格） |
| 3.3V | ESP32-S3、逻辑侧 |
| 板载电源 | STM32 学习板既有供电 |

## PCB/机械约束

- LoRa 天线和 ESP32 天线区域保持净空。
- UART 走线短且远离大电流和继电器驱动区。
- 课程设计阶段优先保证接线可靠性而非量产布线密度。

## DFM/DFT

- 保留 UART 调试口。
- MQTT 主题和 JSON 字段固定，便于联调。
- Android 和 Web 端以相同数据字段展示。
