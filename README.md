# ConnectedDoll ハードウェア

# 概要
ねんどろいど等10cmドールしたに置いてインターネット上にある音声データを再生する装置。  
タッチ操作での反応やフルカラーLEDでの通知にも対応。  
API からのアクセスも可能で、スマートスピーカー連動やSlack連動等拡張にも対応。  

![写真1](./res/dummy.png)

# 環境

## ハードウェア構成　
## 基板 や 部品
- [PCB](https://github.com/ConnectedDoll/pcb)

![基板](/ConnectedDoll/pcb/res/circuit.png)

# 開発環境
- [Visual Studio Code](https://marketplace.visualstudio.com/)
- [PlatformIO IDE](https://platformio.org/platformio-ide)

# ソフトウェア構成　
利用ライブラリやクラウド環境
- [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32)
- [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)
- [arduino-mqtt](https://github.com/256dpi/arduino-mqtt)
- [WiFiManager](https://github.com/zhouhan0126/WIFIMANAGER-ESP32)

- [Google Cloud IoT JWT](https://github.com/GoogleCloudPlatform/google-cloud-iot-arduino)
- [Google Cloud IoT Core](https://cloud.google.com/iot-core?hl=ja)

# ビルド手順

```
cd lib
git clone git@github.com:adafruit/Adafruit_NeoPixel.git
git clone git@github.com:earlephilhower/ESP8266Audio.git
git clone git@github.com:256dpi/arduino-mqtt.git

cd ..
code .
```

# SPIFFS 内設定ファイル
デバイス毎にユニークに発行される値等を初期値としてSPIFFSに書き込む。
アプリ等でアプリのアクティベーション時に `device_id` を利用する。
`data/config.json` 
```
{
    "device_id":"atest-dev",
}
```

# 備考
## Google Cloud IoT での コマンド送信

```
// https://cloud.google.com/sdk/gcloud/reference/iot/devices/commands/send?hl=ja
{"type":"track", "trackname":"/track001.mp3"}
{"type":"url", "url":"https://file-examples.com/wp-content/uploads/2017/11/file_example_MP3_700KB.mp3"}
{"type":"volume", "volume":10}
```

```
gcloud iot devices commands send --region=us-central1 --registry=atest-registry --device=atest-dev --command-data="{\"type\":\"track\",\"trackname\":\"/track001.mp3\"}"
gcloud iot devices commands send --region=us-central1 --registry=atest-registry --device=atest-dev --command-data="{\"type\":\"url\",\"url\":\"https://file-examples.com/wp-content/uploads/2017/11/file_example_MP3_700KB.mp3\"}"
gcloud iot devices commands send --region=us-central1 --registry=atest-registry --device=atest-dev --command-data="{\"type\":\"volume\", \"volume\":10}"

```
