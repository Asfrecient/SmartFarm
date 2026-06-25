package com.example.smartfarm

import android.content.Context
import android.os.Handler
import android.os.Looper
import org.eclipse.paho.client.mqttv3.IMqttActionListener
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.IMqttToken
import org.eclipse.paho.client.mqttv3.MqttAsyncClient
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttException
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class MQTTManager(
    context: Context,
    private val onMessage: (SensorData) -> Unit,
    private val onStatus: (String) -> Unit
) {
    private val mainHandler = Handler(Looper.getMainLooper())
    private val clientId = "${MQTTConfig.CLIENT_ID}-${System.currentTimeMillis()}"
    private val client = MqttAsyncClient(MQTTConfig.BROKER_URI, clientId, MemoryPersistence())
    private val options = MqttConnectOptions().apply {
        isAutomaticReconnect = true
        isCleanSession = true
        connectionTimeout = 5
        keepAliveInterval = 30
    }

    init {
        context.applicationContext
        client.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                postStatus(if (reconnect) "MQTT 已重连" else "MQTT 已连接")
                subscribe()
            }

            override fun connectionLost(cause: Throwable?) {
                postStatus("MQTT 连接断开")
            }

            override fun messageArrived(topic: String?, message: MqttMessage?) {
                if (message == null) return
                try {
                    postMessage(parse(message.toString()))
                } catch (_: Exception) {
                    postStatus("MQTT 数据解析失败")
                }
            }

            override fun deliveryComplete(token: IMqttDeliveryToken?) = Unit
        })
    }

    fun connect(onConnected: (Boolean) -> Unit = {}) {
        if (client.isConnected) {
            onConnected(true)
            return
        }

        postStatus("MQTT 正在连接...")
        try {
            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    postStatus("MQTT 已连接")
                    subscribe()
                    onConnected(true)
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    postStatus("MQTT 连接失败")
                    onConnected(false)
                }
            })
        } catch (_: MqttException) {
            postStatus("MQTT 连接异常")
            onConnected(false)
        }
    }

    private fun subscribe() {
        if (!client.isConnected) return
        try {
            client.subscribe(MQTTConfig.TOPIC, 0)
        } catch (_: MqttException) {
            postStatus("MQTT 订阅失败")
        }
    }

    fun publishPumpCommand(command: PumpCommand, onResult: (Boolean) -> Unit = {}) {
        val payload = when (command) {
            PumpCommand.ON -> JSONObject().put("pump", 1).toString()
            PumpCommand.OFF -> JSONObject().put("pump", 0).toString()
            PumpCommand.AUTO -> JSONObject().put("pump", "auto").toString()
        }
        publishControl(payload, onResult)
    }

    private fun publishControl(payload: String, onResult: (Boolean) -> Unit) {
        if (!client.isConnected) {
            postStatus("MQTT 未连接，控制命令未发送")
            onResult(false)
            return
        }

        try {
            val message = MqttMessage(payload.toByteArray(Charsets.UTF_8)).apply {
                qos = 0
                isRetained = false
            }
            client.publish(MQTTConfig.CONTROL_TOPIC, message, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    postStatus("控制命令已发送")
                    mainHandler.post { onResult(true) }
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    postStatus("控制命令发送失败")
                    mainHandler.post { onResult(false) }
                }
            })
        } catch (_: MqttException) {
            postStatus("控制命令发送异常")
            onResult(false)
        }
    }

    fun close() {
        try {
            if (client.isConnected) {
                client.disconnect()
            }
            client.close()
        } catch (_: MqttException) {
        }
    }

    private fun postMessage(data: SensorData) {
        mainHandler.post { onMessage(data) }
    }

    private fun postStatus(text: String) {
        mainHandler.post { onStatus(text) }
    }

    fun parse(payload: String): SensorData {
        val json = JSONObject(payload)
        return SensorData(
            temp = json.optDouble("temp", 0.0),
            hum = json.optDouble("hum", 0.0),
            soil = json.optInt("soil", 0),
            light = json.optInt("light", 0),
            rain = json.optInt("rain", 0),
            pump = json.optInt("pump", 0),
            pumpManual = json.optInt("pumpManual", 0),
            alarm = json.optInt("alarm", 0),
            timestamp = System.currentTimeMillis()
        )
    }

    fun formatTime(ts: Long = System.currentTimeMillis()): String {
        return SimpleDateFormat("HH:mm:ss", Locale.CHINA).format(Date(ts))
    }
}

enum class PumpCommand {
    ON,
    OFF,
    AUTO
}
