package com.example.smartfarm

data class SensorData(
    val temp: Double = 0.0,
    val hum: Double = 0.0,
    val soil: Int = 0,
    val light: Int = 0,
    val rain: Int = 0,
    val pump: Int = 0,
    val pumpManual: Int = 0,
    val alarm: Int = 0,
    val timestamp: Long = System.currentTimeMillis()
)
