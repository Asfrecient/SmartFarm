package com.example.smartfarm

object MQTTConfig {
    const val BROKER_URI = "tcp://broker.emqx.io:1883"
    const val TOPIC = "smartfarm/data"
    const val CONTROL_TOPIC = "smartfarm/control"
    const val TOPIC_STATUS = "smartfarm/status"
    const val CLIENT_ID = "SmartFarmAndroid"
}
