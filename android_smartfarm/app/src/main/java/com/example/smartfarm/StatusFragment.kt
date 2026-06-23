package com.example.smartfarm

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.example.smartfarm.databinding.FragmentStatusBinding

class StatusFragment : Fragment() {
    private var _binding: FragmentStatusBinding? = null
    private val binding get() = _binding!!
    private var state = StatusUiState()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentStatusBinding.inflate(inflater, container, false)
        render(state)
        return binding.root
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    fun render(text: String) {
        render(state.copy(mqttStatus = text))
    }

    fun render(state: StatusUiState) {
        this.state = state
        _binding ?: return
        binding.statusSummary.text = "${state.dataMode}，最近更新 ${state.latestTime}"
        binding.mqttStatusValue.text = "MQTT：${state.mqttStatus}"
        binding.brokerValue.text = "Broker：${state.broker}"
        binding.topicValue.text = "订阅主题：${state.topic}"
        binding.latestTimeValue.text = "最近更新时间：${state.latestTime}"
        binding.dataModeValue.text = "当前模式：${state.dataMode}"
        binding.dataCountValue.text = "已接收数据：${state.dataCount} 条"
        binding.hardwareDataValue.text = "真实硬件数据：${if (state.hasHardwareData) "是" else "否"}"
    }

    companion object {
        fun newInstance(text: String): StatusFragment {
            return StatusFragment().apply { state = StatusUiState(mqttStatus = text) }
        }
    }
}
