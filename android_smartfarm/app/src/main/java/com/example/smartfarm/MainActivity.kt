package com.example.smartfarm

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.example.smartfarm.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private var mqttManager: MQTTManager? = null
    private val store = SensorStore()
    private val dashboardFragment = DashboardFragment.newInstance(null)
    private val historyFragment = HistoryFragment.newInstance(emptyList())
    private val statusFragment = StatusFragment.newInstance("等待 MQTT 连接")
    private var currentTabId: Int = R.id.nav_dashboard
    private var demoMode = true
    private var mqttStatus = "等待 MQTT 连接"
    private var lastUpdateTime = "--"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        if (savedInstanceState == null) {
            supportFragmentManager.beginTransaction()
                .replace(R.id.fragmentContainer, dashboardFragment)
                .commit()
            currentTabId = R.id.nav_dashboard
        }

        binding.bottomNav.setOnItemSelectedListener { item ->
            if (item.itemId == currentTabId) {
                return@setOnItemSelectedListener true
            }
            val fragment = when (item.itemId) {
                R.id.nav_dashboard -> dashboardFragment
                R.id.nav_history -> historyFragment
                else -> statusFragment
            }
            currentTabId = item.itemId
            supportFragmentManager.beginTransaction()
                .replace(R.id.fragmentContainer, fragment)
                .commit()
            true
        }

        dashboardFragment.setPumpControlListener(object : PumpControlListener {
            override fun onPumpCommand(command: PumpCommand) {
                sendPumpCommand(command)
            }
        })
        startDemoMode()
        startMqttMode()

        dashboardFragment.render(store.latest())
        historyFragment.render(store.items())
        renderStatus()
    }

    override fun onDestroy() {
        mqttManager?.close()
        super.onDestroy()
    }

    private fun startDemoMode() {
        seedDemoData()
        demoMode = true
    }

    private fun startMqttMode() {
        mqttManager = MQTTManager(
            context = this,
            onMessage = { data ->
                demoMode = false
                mqttStatus = "MQTT 已接收数据"
                lastUpdateTime = mqttManager?.formatTime(data.timestamp) ?: "--"
                store.add(data)
                dashboardFragment.render(data)
                historyFragment.render(store.items())
                renderStatus()
            },
            onStatus = { status ->
                mqttStatus = status
                renderStatus()
            }
        )
        mqttManager?.connect()
    }

    private fun seedDemoData() {
        store.add(SensorData(temp = 25.6, hum = 68.0, soil = 42, light = 520, rain = 0, pump = 0, pumpManual = 0, alarm = 0))
        store.add(SensorData(temp = 26.1, hum = 66.4, soil = 39, light = 540, rain = 0, pump = 0, pumpManual = 0, alarm = 0))
        store.add(SensorData(temp = 27.0, hum = 64.8, soil = 36, light = 560, rain = 0, pump = 1, pumpManual = 1, alarm = 0))
    }

    private fun sendPumpCommand(command: PumpCommand) {
        val text = when (command) {
            PumpCommand.ON -> "正在发送：手动开启水泵"
            PumpCommand.OFF -> "正在发送：手动关闭水泵"
            PumpCommand.AUTO -> "正在发送：恢复自动控制"
        }
        dashboardFragment.setPumpCommandStatus(text)
        mqttManager?.publishPumpCommand(command) { success ->
            dashboardFragment.setPumpCommandStatus(if (success) "命令已发送，等待设备状态回传" else "命令发送失败，请检查 MQTT 连接")
        } ?: dashboardFragment.setPumpCommandStatus("MQTT 未初始化，命令未发送")
    }

    private fun renderStatus() {
        statusFragment.render(
            StatusUiState(
                mqttStatus = mqttStatus,
                latestTime = lastUpdateTime,
                dataMode = if (demoMode) "演示模式" else "在线模式",
                dataCount = store.count(),
                hasHardwareData = !demoMode
            )
        )
    }
}
