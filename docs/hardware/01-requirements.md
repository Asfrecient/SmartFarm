# SmartFarm 课程设计报告
# 阶段 1：需求冻结
# 日期：2026-06-23

## 工程约束表

| 项目 | 约束 |
|---|---|
| 场景 | 温室/农场环境监测与自动灌溉 |
| 主控 | STM32F103C8T6，复用现有工程 |
| 系统 | FreeRTOS，不重构任务框架 |
| 传感器 | AHT20、土壤湿度、光照、雨滴 |
| 执行器 | 水泵继电器、蜂鸣器、OLED、RGB灯 |
| 通信 | STM32 + E32-433T20D + ESP32-S3 + WiFi + MQTT |
| Web | Node-RED Dashboard |
| Android | Android Studio + MQTT 客户端 |
| MQTT Broker | broker.emqx.io:1883 |
| Topic | smartfarm/data |
| 供电 | 现有学习板供电体系，LoRa 模块 5V/3.3V 按器件要求接入 |
| 环境 | 教学/实验室为主，按课程设计答辩准备 |
| 认证 | 不做量产认证，仅保留接口与风险说明 |

## 优先级选择

| 选项 | 说明 |
|---|---|
| 快速落地 | 以课程验收为目标，优先打通链路 |
| 复用优先 | 保留 STM32 传感器驱动和 FreeRTOS 结构 |
| 展示完整 | 同时提供 Web 和 Android 两端展示 |

## 决策记录

- 已确认采用 LoRa 作为 STM32 到网关的无线链路。
- 已确认采用 ESP32-S3 作为 WiFi/MQTT 网关。
- 已确认采用 Node-RED 作为 Web 监控端。
- 已确认采用 Android Studio 客户端作为移动端展示。
- 已确认 MQTT 数据主题为 `smartfarm/data`。

## 需求结论

当前方案以“课设可交付”为主，不追求量产级硬件重构；系统边界已经冻结，可进入架构与实现阶段。
