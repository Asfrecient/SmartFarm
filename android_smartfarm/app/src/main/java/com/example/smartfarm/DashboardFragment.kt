package com.example.smartfarm

import android.os.Bundle
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.example.smartfarm.databinding.FragmentDashboardBinding
import java.text.SimpleDateFormat
import java.util.Locale

class DashboardFragment : Fragment() {
    private var _binding: FragmentDashboardBinding? = null
    private val binding get() = _binding!!
    private var latest: SensorData? = null
    private var listener: PumpControlListener? = null
    private var suppressPumpSwitchCallback = false

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentDashboardBinding.inflate(inflater, container, false)
        binding.pumpControlSwitch.setOnCheckedChangeListener { _, isChecked ->
            if (!suppressPumpSwitchCallback) {
                listener?.onPumpCommand(if (isChecked) PumpCommand.ON else PumpCommand.OFF)
            }
        }
        binding.pumpAutoButton.setOnClickListener {
            listener?.onPumpCommand(PumpCommand.AUTO)
        }
        render(latest)
        return binding.root
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    fun render(data: SensorData?) {
        latest = data
        _binding ?: return
        val value = data ?: SensorData()
        binding.tempValue.text = String.format("%.1f °C", value.temp)
        binding.humValue.text = String.format("%.1f %%", value.hum)
        binding.soilValue.text = "${value.soil} %"
        binding.lightValue.text = "${value.light} lx"
        binding.latestTimeValue.text = if (data == null) {
            "等待 MQTT 数据"
        } else {
            "更新时间 ${timeOf(value.timestamp)}"
        }
        binding.environmentSummary.text = "温度 ${formatOne(value.temp)}°C，湿度 ${formatOne(value.hum)}%，降雨 ${value.rain}%"
        binding.controlSummary.text = when {
            value.alarm > 0 -> "系统报警，请检查阈值和现场设备"
            value.pump > 0 -> "水泵正在灌溉，土壤湿度持续监测中"
            else -> "水泵关闭，系统运行正常"
        }
        binding.rainValue.text = "降雨 ${value.rain}%"
        binding.pumpValue.text = if (value.pump > 0) "水泵 开启" else "水泵 关闭"
        binding.alarmValue.text = if (value.alarm > 0) "告警 报警" else "告警 正常"
        binding.pumpModeValue.text = if (value.pumpManual > 0) "当前：手动模式" else "当前：自动模式"
        suppressPumpSwitchCallback = true
        binding.pumpControlSwitch.isChecked = value.pump > 0
        suppressPumpSwitchCallback = false
        binding.rainValue.setTextColor(if (value.rain >= 80) Color.parseColor("#0D5C8A") else Color.parseColor("#46715F"))
        binding.pumpValue.setTextColor(if (value.pump > 0) Color.parseColor("#1F7A39") else Color.parseColor("#6E7B72"))
        binding.alarmValue.setTextColor(if (value.alarm > 0) Color.parseColor("#B3261E") else Color.parseColor("#2E6C3F"))
    }

    fun setPumpControlListener(listener: PumpControlListener) {
        this.listener = listener
    }

    fun setPumpCommandStatus(text: String) {
        _binding ?: return
        binding.pumpCommandStatus.text = text
    }

    private fun timeOf(timestamp: Long): String {
        return SimpleDateFormat("HH:mm:ss", Locale.CHINA).format(timestamp)
    }

    private fun formatOne(value: Double): String {
        return String.format(Locale.CHINA, "%.1f", value)
    }

    companion object {
        fun newInstance(data: SensorData?): DashboardFragment {
            return DashboardFragment().apply { latest = data }
        }
    }
}

interface PumpControlListener {
    fun onPumpCommand(command: PumpCommand)
}
