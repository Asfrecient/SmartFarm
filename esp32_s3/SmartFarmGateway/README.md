# SmartFarmGateway

这是 ESP32-S3 的 Arduino 工程目录。

打开方式：

1. 用 Arduino IDE 直接打开这个文件夹
2. 修改 `SmartFarmGateway.ino` 里的 WiFi 名称和密码
3. 选择 ESP32-S3 对应板卡后编译上传

## iPhone 热点

可以使用 iPhone 手机热点。建议在 iPhone 上打开：

1. 设置 -> 个人热点 -> 允许其他人加入
2. 设置 -> 个人热点 -> 最大兼容性

然后把 `SmartFarmGateway.ino` 中的下面两行改成手机热点名称和密码：

```cpp
static const char* WIFI_SSID = "你的iPhone热点名称";
static const char* WIFI_PASSWORD = "你的热点密码";
```

如果热点名称里有中文、空格或特殊符号也可以使用，但最稳妥的是临时改成纯英文名称。

## 烧录后检查

1. 打开 Arduino IDE 串口监视器
2. 波特率选择 `115200`
3. 按一下 ESP32-S3 的 `RST/EN`
4. 正常会看到中文日志，例如：

```text
智慧农场 ESP32-S3 网关启动。
正在连接 WiFi：...
WiFi 已连接。
MQTT 已连接。
```

收到 LoRa 数据后，会打印数据内容，并发布到 MQTT 主题 `smartfarm/data`。
网关还会订阅 MQTT 主题 `smartfarm/control`，收到 Android 发来的 `{"pump":1}`、`{"pump":0}` 或 `{"pump":"auto"}` 后，通过 LoRa 下发给 STM32。
