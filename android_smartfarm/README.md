# SmartFarm Android 工程

打开方式：

1. 用 Android Studio 选择 `android_smartfarm` 目录
2. 等待 Gradle 同步
3. 如果同步失败，先检查 JDK 是否为 17

工程说明：

- 主页面：实时监控、历史曲线、系统状态
- 数据源：MQTT `smartfarm/data`
- 控制源：MQTT `smartfarm/control`
- 主题：EMQX Public Broker

水泵控制：

- 手动开启：`{"pump":1}`
- 手动关闭：`{"pump":0}`
- 恢复自动：`{"pump":"auto"}`
