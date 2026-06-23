package com.example.smartfarm

class SensorStore {
    private val history = ArrayDeque<SensorData>()
    private val maxSize = 50

    fun add(data: SensorData) {
        if (history.size >= maxSize) {
            history.removeFirst()
        }
        history.addLast(data)
    }

    fun latest(): SensorData? = history.lastOrNull()

    fun items(): List<SensorData> = history.toList().reversed()

    fun count(): Int = history.size
}
