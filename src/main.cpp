#include <Arduino.h>
#include <ArduinoOSC.h>

#include "SPI.h"
#include "I2S.h"
#include "FS.h"
#include "SPIFFS.h"
#include "LedUtil.h"
#include "WifiUtil.h"

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#include "FileDownloader.h"
#include "SpiffsUtil.h"

#include "ESPAsyncWebServer.h"

WifiUtil wifiUtil;
// AP モード SSID
const char* WIFI_SSID = "ConnectedDoll";
// mDNS ホスト名
const char* MDNS_HOST = "connecteddoll";
// OSC受信 設定
const int OSC_BIND_PORT = 54345;

LedUtil ledUtil;

// ステータス
struct DEVICE_STATUS {
  float volume;
  String mode;
  String path;
  String color[4];
};
DEVICE_STATUS status;

#define DEVICE_STATUS_MODE_DEFAULT "default"
#define DEVICE_STATUS_MODE_RESET   "reset"
#define DEVICE_STATUS_MODE_DIR     "dir"
#define DEVICE_STATUS_MODE_PLAY_MP3    "play_mp3" 
#define DEVICE_STATUS_MODE_PLAY_URL_MP3    "play_url_mp3" 
#define DEVICE_STATUS_MODE_COLOR   "color"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

SpiffsUtil spiffsUtil;

FileDownloader fileDownloader;
// ファイルダウンロード時の読み込みサイズ
#define DOWNLOAD_FILE_SIZE 512
uint8_t downloadFileData[DOWNLOAD_FILE_SIZE];
// ダウンロードするファイルへつける拡張子
#define AUDIO_FILE_EXTENSION "mp3"

const int HTTP_PORT = 80;
AsyncWebServer webServer(HTTP_PORT);

String processor(const String& var){
  Serial.print("processor "); 
  Serial.println(var);
  return "";
}

void webServerSetup() {
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.html", "text/html");
  });
  webServer.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.css", "text/css");
  });
  webServer.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.js", "text/javascript");
  });
  webServer.on("/300x100.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/300x100.png", "image/png");
  });
  webServer.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/index.html", "text/html", false, processor);
  });
  webServer.begin();
}

const char* downloadFile(const char* url) { 
  Serial.printf("downloadFile: %s\n", url);
  fileDownloader.open(url);
  // URLからハッシュファイル名生成
  const char* fileName = fileDownloader.getHashFileName(AUDIO_FILE_EXTENSION);
  Serial.printf("fileName: %s\n", fileName);
  // sdUtil.remove(fileName);
  // 既にあったらダウンロード処理をしない
  if (spiffsUtil.exists(fileName)){
    fileDownloader.close();
    return fileName;
  }

  File fw = spiffsUtil.open(fileName, FILE_WRITE);
  if (!fw) {
    Serial.println(F("There was an error opening the file for writing"));
    while (1);
  }
  uint32_t size = fileDownloader.getSize();
  uint32_t pos = 0;
  while (pos < size){
    int r = fileDownloader.read(downloadFileData, DOWNLOAD_FILE_SIZE);
    pos = fileDownloader.getPos();
    // Serial.printf(" read:%d pos:%d size:%d\n", r, pos, size);
    // SPIFFS にファイル書き込み
    int w = spiffsUtil.write(downloadFileData, r);
  }
  Serial.printf(" pos:%d size:%d\n", pos, size);
  spiffsUtil.close();
  fileDownloader.close();

  return fileName;
}

void playMp3(const char* spiffsFile){
  Serial.println("playMp3"); 
  file = new AudioFileSourceSPIFFS(spiffsFile);
  if (!file) {
    Serial.print("file is null");
    while (1);
  }
  Serial.print("file size:"); Serial.println( file->getSize() );
  Serial.printf("volume %.1f\n", status.volume);
  if (file->getSize() == 0) return;

  out = new AudioOutputI2S();
  out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  // out->SetChannels(1);
  out->SetGain(status.volume);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  while(mp3->isRunning()){
    if (!mp3->loop()) mp3->stop();
  }
}
// URL指定されたMP3ファイルをダウンロードして再生する
void playUrlMP3(const char* url){
  // SPIFFS にファイルをダウンロードする
  const char* fireName = downloadFile(url);
  playMp3(fireName);
}

//ステータス初期化
void setupStatus(){
  status.mode = DEVICE_STATUS_MODE_DEFAULT;
  status.volume = 0.3f;
}

// OSC 初期化
void oscWiFiSetup() {
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/reset",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_RESET;
      Serial.printf("/status/reset\n");
    }
  );
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/dir",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_DIR;
      Serial.printf("/status/dir\n");
    }
  );  
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/volume",
    [](const OscMessage& m)
    {
      float v = m.arg<float>(0);
      status.volume = v;
      Serial.printf("/status/volume %.1f\n", status.volume);
    }
  );

  // SPIFFS MP3ファイル再生
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/play/mp3",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY_MP3;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play/mp3 %s\n", status.path.c_str());
    }
  );  

  // URL MP3ファイル再生
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/play/url/mp3",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY_URL_MP3;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play/url/mp3 %s\n", status.path.c_str());
    }
  );

  // LED カラー設定
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/color",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_COLOR;
      status.color[0] = m.arg<String>(0);
      status.color[1] = m.arg<String>(1);
      status.color[2] = m.arg<String>(2);
      status.color[3] = m.arg<String>(3);

      Serial.printf("/status/color %s %s %s %s\n",
          status.color[0].c_str(),
          status.color[1].c_str(),
          status.color[2].c_str(),
          status.color[3].c_str());
    }
  );
}

void setup() {
  Serial.begin(115200);
  
  // SPIFFS 初期化
  spiffsUtil.begin();

  // LED 初期化
  ledUtil.setup(LED1);

  //ステータス初期化
  setupStatus();

  // WiFi 接続設定
  wifiUtil.setupWiFi(WIFI_SSID);
  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());
  // mDNS 設定
  wifiUtil.setupMDNS(MDNS_HOST, HTTP_PORT, OSC_BIND_PORT);
  
  // WebServerSetup
  webServerSetup();

  // OSC受信設定
  oscWiFiSetup();
}

// OSC コマンド受付
void loopOscCmd() {
  OscWiFi.update();
  if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_RESET) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    wifiUtil.resetSettings();
    ESP.restart();
  }
  else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_DIR) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    spiffsUtil.listDir("/", 0);
  }
  else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY_MP3) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    //SPIFFS mp3 ファイル 再生
    playMp3(status.path.c_str());
  }
  else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY_URL_MP3) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    //URL ファイル 再生 ダウンロードし再生する
    playUrlMP3(status.path.c_str());
  } else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_COLOR) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    // LED 色設定
    ledUtil.setPixelColor(0, status.color[0].c_str());
    ledUtil.setPixelColor(1, status.color[1].c_str());
    ledUtil.setPixelColor(2, status.color[2].c_str());
    ledUtil.setPixelColor(3, status.color[3].c_str());
    ledUtil.show();
  }
}

void loop() {
  delay(200);
  // Serial.println("loop");
  // Serial.print("IP = "); Serial.println(WiFi.localIP());
  // Serial.print("HOST = "); Serial.print(MDNS_HOST); Serial.println(".local");
  loopOscCmd();
}
