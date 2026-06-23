package com.example.smartfarm

data class StatusUiState(
    val mqttStatus: String = "等待 MQTT 连接",
    val broker: String = MQTTConfig.BROKER_URI,
    val topic: String = MQTTConfig.TOPIC,
    val latestTime: String = "--",
    val dataMode: String = "演示模式",
    val dataCount: Int = 0,
    val hasHardwareData: Boolean = false
)
