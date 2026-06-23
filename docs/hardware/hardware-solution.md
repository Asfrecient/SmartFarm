# SmartFarm 课程设计报告

## 项目概述

本课设实现基于 LoRa 的智慧农业系统：STM32 负责采集和控制，ESP32-S3 负责网关转发，Node-RED 负责 Web 监控，Android 负责移动端展示。

## 系统组成

- STM32F103C8T6 + FreeRTOS
- E32-433T20D LoRa 模块
- ESP32-S3-N16R8 网关
- EMQX Public Broker
- Node-RED Dashboard
- Android Studio 客户端

## 数据流

1. STM32 采集温湿度、土壤湿度、光照、雨滴。
2. STM32 通过 UART3 将 JSON 发往 E32。
3. ESP32-S3 接收 JSON 并发布 MQTT 到 `smartfarm/data`。
4. Node-RED 和 Android 订阅该主题展示。

## 实现结果

- STM32 已保留原有传感器驱动与 FreeRTOS 框架。
- ESP32-S3 已整理为 Arduino IDE 可直接打开的网关工程。
- Android 已完成实时监控、历史曲线、系统状态三页，并支持 MQTT 在线模式。
- Web 已给出 Node-RED 流配置。
- 联调步骤已整理到项目根目录 `联调步骤.md`。

## 结论

本方案满足课程设计的“LoRa + MQTT + Web + Android”要求，且最大程度复用了现有 SmartFarm 工程。
