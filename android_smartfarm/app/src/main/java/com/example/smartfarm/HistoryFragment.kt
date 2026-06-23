package com.example.smartfarm

import android.os.Bundle
import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TableRow
import android.widget.TextView
import androidx.fragment.app.Fragment
import com.example.smartfarm.databinding.FragmentHistoryBinding
import com.github.mikephil.charting.components.AxisBase
import com.github.mikephil.charting.components.XAxis
import com.github.mikephil.charting.data.Entry
import com.github.mikephil.charting.data.LineData
import com.github.mikephil.charting.data.LineDataSet
import com.github.mikephil.charting.formatter.ValueFormatter
import java.text.SimpleDateFormat
import java.util.Locale

class HistoryFragment : Fragment() {
    private var _binding: FragmentHistoryBinding? = null
    private val binding get() = _binding!!
    private var records: List<SensorData> = emptyList()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentHistoryBinding.inflate(inflater, container, false)
        render(records)
        return binding.root
    }

    override fun onDestroyView() {
        _binding = null
        super.onDestroyView()
    }

    fun render(items: List<SensorData>) {
        records = items
        _binding ?: return

        if (items.isEmpty()) {
            binding.lineChart.clear()
            binding.historyCount.text = "最近 0 / 10 条记录"
            renderTable(emptyList())
            return
        }

        val visibleItems = items.take(10)
        val chartItems = visibleItems.reversed()
        val tempSet = lineSet(
            label = "温度(°C)",
            color = Color.parseColor("#D85B45"),
            entries = chartItems.mapIndexed { index, data -> Entry(index.toFloat(), data.temp.toFloat()) }
        )
        val humSet = lineSet(
            label = "湿度(%)",
            color = Color.parseColor("#2C7FB8"),
            entries = chartItems.mapIndexed { index, data -> Entry(index.toFloat(), data.hum.toFloat()) }
        )
        val soilSet = lineSet(
            label = "土壤(%)",
            color = Color.parseColor("#3A8B5B"),
            entries = chartItems.mapIndexed { index, data -> Entry(index.toFloat(), data.soil.toFloat()) }
        )
        binding.lineChart.apply {
            data = LineData(tempSet, humSet, soilSet)
            description.isEnabled = false
            legend.textSize = 11f
            axisRight.isEnabled = false
            axisLeft.axisMinimum = 0f
            axisLeft.textColor = Color.parseColor("#66736B")
            axisLeft.gridColor = Color.parseColor("#E7EEE8")
            xAxis.position = XAxis.XAxisPosition.BOTTOM
            xAxis.textColor = Color.parseColor("#66736B")
            xAxis.gridColor = Color.TRANSPARENT
            xAxis.granularity = 1f
            xAxis.labelCount = minOf(6, chartItems.size)
            xAxis.valueFormatter = object : ValueFormatter() {
                override fun getAxisLabel(value: Float, axis: AxisBase?): String {
                    val index = value.toInt()
                    if (index !in chartItems.indices) return ""
                    return timeOf(chartItems[index].timestamp).substring(0, 5)
                }
            }
            setNoDataText("暂无历史数据")
            setTouchEnabled(true)
            setPinchZoom(true)
            invalidate()
        }
        binding.historyCount.text = "最近 ${visibleItems.size} / 10 条记录"
        renderTable(visibleItems)
    }

    private fun lineSet(label: String, color: Int, entries: List<Entry>): LineDataSet {
        return LineDataSet(entries, label).apply {
            this.color = color
            setCircleColor(color)
            valueTextSize = 0f
            lineWidth = 2.2f
            circleRadius = 2.5f
            setDrawCircleHole(false)
            setDrawValues(false)
            mode = LineDataSet.Mode.CUBIC_BEZIER
        }
    }

    private fun renderTable(items: List<SensorData>) {
        binding.historyTable.removeAllViews()
        addTableRow(
            listOf("时间", "温度", "湿度", "土壤", "雨量", "光照", "泵", "警"),
            isHeader = true
        )
        if (items.isEmpty()) {
            addTableRow(listOf("暂无数据", "-", "-", "-", "-", "-", "-", "-"))
            return
        }
        items.forEach {
            addTableRow(
                listOf(
                    timeOf(it.timestamp),
                    "${formatOne(it.temp)}°C",
                    "${formatOne(it.hum)}%",
                    "${it.soil}%",
                    "${it.rain}%",
                    "${it.light}lx",
                    if (it.pump > 0) "开" else "关",
                    if (it.alarm > 0) "报" else "正"
                )
            )
        }
    }

    private fun addTableRow(values: List<String>, isHeader: Boolean = false) {
        val row = TableRow(requireContext()).apply {
            setPadding(0, 4, 0, 4)
        }
        values.forEachIndexed { index, value ->
            row.addView(TextView(requireContext()).apply {
                text = value
                textSize = if (isHeader) 12f else 11f
                setTextColor(Color.parseColor(if (isHeader) "#1E2F25" else "#405047"))
                setTypeface(typeface, if (isHeader) android.graphics.Typeface.BOLD else android.graphics.Typeface.NORMAL)
                gravity = if (index == 0) android.view.Gravity.START else android.view.Gravity.CENTER
                minWidth = when (index) {
                    0 -> dp(78)
                    5 -> dp(66)
                    else -> dp(58)
                }
                setPadding(dp(4), dp(6), dp(4), dp(6))
                layoutParams = TableRow.LayoutParams(TableRow.LayoutParams.WRAP_CONTENT, TableRow.LayoutParams.WRAP_CONTENT)
            })
        }
        binding.historyTable.addView(row)
    }

    private fun timeOf(timestamp: Long): String {
        return SimpleDateFormat("HH:mm:ss", Locale.CHINA).format(timestamp)
    }

    private fun formatOne(value: Double): String {
        return String.format(Locale.CHINA, "%.1f", value)
    }

    private fun dp(value: Int): Int {
        return (value * resources.displayMetrics.density).toInt()
    }

    companion object {
        fun newInstance(items: List<SensorData>): HistoryFragment {
            return HistoryFragment().apply { records = items }
        }
    }
}
