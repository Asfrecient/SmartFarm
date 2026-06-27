package com.example.smartfarm

object MQTTConfig {
    val BROKER_URIS = arrayOf(
        "tcp://35.172.255.228:1883",
        "tcp://34.243.217.54:1883",
        "tcp://44.232.241.40:1883",
        "tcp://broker.emqx.io:1883"
    )
    const val BROKER_URI = "tcp://35.172.255.228:1883"
    const val TOPIC = "smartfarm/data"
    const val CONTROL_TOPIC = "smartfarm/control"
    const val TOPIC_STATUS = "smartfarm/status"
    const val CLIENT_ID = "SmartFarmAndroid"
}
