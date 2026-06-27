package com.example.smartfarm

import android.content.Context
import android.net.ConnectivityManager
import android.net.Network
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
    private val appContext = context.applicationContext
    private val mainHandler = Handler(Looper.getMainLooper())
    private val retryHandler = Handler(Looper.getMainLooper())
    private val connectivityManager =
        appContext.getSystemService(ConnectivityManager::class.java)
    private val clientId = "${MQTTConfig.CLIENT_ID}-${System.currentTimeMillis()}"
    private val client = MqttAsyncClient(MQTTConfig.BROKER_URI, clientId, MemoryPersistence())
    private val options = MqttConnectOptions().apply {
        isAutomaticReconnect = true
        isCleanSession = true
        connectionTimeout = 20
        keepAliveInterval = 30
        serverURIs = MQTTConfig.BROKER_URIS
    }
    @Volatile private var connectInFlight = false
    @Volatile private var closed = false
    private var retryDelayMs = 2000L
    private val retryRunnable = Runnable {
        if (!closed && !client.isConnected && !connectInFlight) {
            connect()
        }
    }
    private val networkCallback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) {
            android.util.Log.d("SmartFarmMQTT", "[NETWORK_AVAILABLE] reconnecting")
            if (!client.isConnected && !connectInFlight) {
                scheduleReconnect(0)
            }
        }

        override fun onLost(network: Network) {
            android.util.Log.d("SmartFarmMQTT", "[NETWORK_LOST]")
        }
    }

    init {
        client.setCallback(object : MqttCallbackExtended {
            override fun connectComplete(reconnect: Boolean, serverURI: String?) {
                android.util.Log.d("SmartFarmMQTT", "[CONNECT_COMPLETE] reconnect=$reconnect serverURI=$serverURI")
                postStatus(if (reconnect) "MQTT 已重连" else "MQTT 已连接")
                subscribe()
            }

            override fun connectionLost(cause: Throwable?) {
                postStatus("MQTT 连接断开${cause.toDebugText()}")
            }

            override fun messageArrived(topic: String?, message: MqttMessage?) {
                if (message == null) return
                try {
                    android.util.Log.d("SmartFarmMQTT", "[MESSAGE] topic=$topic payload=${message.toString()}")
                    postMessage(parse(message.toString()))
                } catch (_: Exception) {
                    android.util.Log.e("SmartFarmMQTT", "[MESSAGE_PARSE_FAIL] topic=$topic payload=${message.toString()}")
                    postStatus("MQTT 数据解析失败")
                }
            }

            override fun deliveryComplete(token: IMqttDeliveryToken?) = Unit
        })

        try {
            connectivityManager?.registerDefaultNetworkCallback(networkCallback)
        } catch (exception: Exception) {
            android.util.Log.w("SmartFarmMQTT", "[NETWORK_CALLBACK_FAIL] ${exception.toDebugText()}")
        }
    }

    fun connect(onConnected: (Boolean) -> Unit = {}) {
        if (client.isConnected) {
            onConnected(true)
            return
        }
        if (connectInFlight) {
            return
        }

        connectInFlight = true
        postStatus("MQTT 正在连接... ${MQTTConfig.BROKER_URI}")
        android.util.Log.d("SmartFarmMQTT", "[CONNECT] brokerCandidates=${MQTTConfig.BROKER_URIS.joinToString()} clientId=$clientId")
        try {
            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    connectInFlight = false
                    retryDelayMs = 2000L
                    android.util.Log.d("SmartFarmMQTT", "[CONNECT_OK] broker=${MQTTConfig.BROKER_URI}")
                    postStatus("MQTT 已连接")
                    onConnected(true)
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    connectInFlight = false
                    android.util.Log.e("SmartFarmMQTT", "[CONNECT_FAIL] brokerCandidates=${MQTTConfig.BROKER_URIS.joinToString()} ${exception.toDebugText()}")
                    postStatus("MQTT 连接失败${exception.toDebugText()}，稍后重试")
                    onConnected(false)
                    scheduleReconnect()
                }
            })
        } catch (exception: MqttException) {
            connectInFlight = false
            postStatus("MQTT 连接异常${exception.toDebugText()}")
            onConnected(false)
            scheduleReconnect()
        }
    }

    private fun subscribe() {
        if (!client.isConnected) return
        try {
            android.util.Log.d("SmartFarmMQTT", "[SUBSCRIBE] topic=${MQTTConfig.TOPIC}")
            client.subscribe(MQTTConfig.TOPIC, 0)
        } catch (exception: MqttException) {
            android.util.Log.e("SmartFarmMQTT", "[SUBSCRIBE_FAIL] ${exception.toDebugText()}")
            postStatus("MQTT 订阅失败${exception.toDebugText()}")
        }
    }

    private fun scheduleReconnect(delayMs: Long = retryDelayMs) {
        if (closed || client.isConnected || connectInFlight) return
        retryHandler.removeCallbacks(retryRunnable)
        android.util.Log.d("SmartFarmMQTT", "[RETRY_SCHEDULE] delayMs=$delayMs")
        postStatus("MQTT 连接重试中，${delayMs / 1000} 秒后重连")
        retryHandler.postDelayed(retryRunnable, delayMs)
        retryDelayMs = (delayMs * 2).coerceAtMost(30_000L)
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
                    postStatus("控制命令发送失败${exception.toDebugText()}")
                    mainHandler.post { onResult(false) }
                }
            })
        } catch (exception: MqttException) {
            postStatus("控制命令发送异常${exception.toDebugText()}")
            onResult(false)
        }
    }

    fun close() {
        closed = true
        retryHandler.removeCallbacksAndMessages(null)
        try {
            connectivityManager?.unregisterNetworkCallback(networkCallback)
        } catch (_: Exception) {
        }
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
            rx = json.optLong("rx", 0L),
            cmd = json.optInt("cmd", 0),
            timestamp = System.currentTimeMillis()
        )
    }

    fun formatTime(ts: Long = System.currentTimeMillis()): String {
        return SimpleDateFormat("HH:mm:ss", Locale.CHINA).format(Date(ts))
    }

    private fun Throwable?.toDebugText(): String {
        if (this == null) return ""
        val type = javaClass.simpleName
        val reason = if (this is MqttException) " reason=${reasonCode}" else ""
        val messageText = message?.takeIf { it.isNotBlank() }?.let { " msg=$it" } ?: ""
        val causeText = cause?.javaClass?.simpleName?.let { " cause=$it" } ?: ""
        return "[$type$reason$messageText$causeText]"
    }
}

enum class PumpCommand {
    ON,
    OFF,
    AUTO
}
