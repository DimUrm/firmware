#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "SPI.h"
#include "I2S.h"
#include "FS.h"
#include "SPIFFS.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>

#include <ArduinoOSC.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

#include "FileDownloader.h"
#include "SpiffsUtil.h"

// WiFi 設定
WiFiManager wifiManager;

// OSC受信 設定
const char* localhost = "127.0.0.1";
const int bind_port = 54345;

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
AudioGeneratorWAV *wav;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

#define LED1 13

#define RGB_LED 4
#define NUMPIXELS 4
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);

const int offset = 0x2C;
char data[800];
char stereoData[1600];

SpiffsUtil spiffsUtil;

FileDownloader fileDownloader;
// ファイルダウンロード時の読み込みサイズ
#define DOWNLOAD_FILE_SIZE 512
uint8_t downloadFileData[DOWNLOAD_FILE_SIZE];
// ダウンロードするファイルへつける拡張子
#define AUDIO_FILE_EXTENSION "mp3"

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setupWiFi() {
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.autoConnect("ConnectedDoll");
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

// "r,g,b" を int[3] に変換する
void rgbStr2rgbInt(const char* src, int rgb[]) {
  char buf[20];
  strcpy(buf,src);
  rgb[0] = atoi(strtok(buf, ","));
  rgb[1] = atoi(strtok(NULL, ","));
  rgb[2] = atoi(strtok(NULL, ","));
}

void setup() {
  Serial.begin(115200);
  
  // SPIFFS 初期化
  spiffsUtil.begin();

  pinMode(LED1, OUTPUT);

  pixels.begin();
  pixels.clear();

  //ステータス初期化
  setupStatus();

  // WiFi 接続設定
  setupWiFi();

  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());

  // OSC受信設定
  OscWiFi.subscribe(bind_port, "/status/reset",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_RESET;
      Serial.printf("/status/reset\n");
    }
  );
  OscWiFi.subscribe(bind_port, "/status/dir",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_DIR;
      Serial.printf("/status/dir\n");
    }
  );  
  OscWiFi.subscribe(bind_port, "/status/volume",
    [](const OscMessage& m)
    {
      float v = m.arg<float>(0);
      status.volume = v;
      Serial.printf("/status/volume %f\n", status.volume);
    }
  );

  // SPIFFS MP3ファイル再生
  OscWiFi.subscribe(bind_port, "/status/play/mp3",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY_MP3;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play/mp3 %s\n", status.path.c_str());
    }
  );  

  // URL MP3ファイル再生
  OscWiFi.subscribe(bind_port, "/status/play/url/mp3",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY_URL_MP3;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play/url/mp3 %s\n", status.path.c_str());
    }
  );

  // LED カラー設定
  OscWiFi.subscribe(bind_port, "/status/color",
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

void loop() {
  delay(200);
  Serial.println("loop");
  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());
  OscWiFi.update();
  if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_RESET) == 0) {
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    wifiManager.resetSettings();
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
    int rgb[3];
    rgbStr2rgbInt(status.color[0].c_str(),rgb);
    pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[1].c_str(),rgb);
    pixels.setPixelColor(1, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[2].c_str(),rgb);
    pixels.setPixelColor(2, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[3].c_str(),rgb);
    pixels.setPixelColor(3, pixels.Color(rgb[0], rgb[1], rgb[2]));
    pixels.show();
  }
}
