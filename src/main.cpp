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
#define DEVICE_STATUS_MODE_PLAY    "play" 
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

void playMP3(const char* spiffsFile){
  Serial.println("playWAV2"); 
  file = new AudioFileSourceSPIFFS(spiffsFile);
  if (!file) {
    Serial.print("file is null");
    while (1);
  }
  Serial.print("file size:"); Serial.println( file->getSize() );

  out = new AudioOutputI2S();
  out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  // out->SetChannels(1);
  // out->SetGain(0.3);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  while(mp3->isRunning()){
    if (!mp3->loop()) mp3->stop();
  }
}

void playWAV2(const char* spiffsFile){
  Serial.println("playWAV2"); 
  file = new AudioFileSourceSPIFFS(spiffsFile);
  if (!file) {
    Serial.print("file is null");
    while (1);
  }
  Serial.print("file size:"); Serial.println( file->getSize() );

  out = new AudioOutputI2S();
  out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  // out->SetChannels(1);
  out->SetGain(status.volume);
  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
  while(wav->isRunning()){
    if (!wav->loop()) wav->stop();
  }
}

void dirSPIFFS(){
  Serial.println("dir");
  String fileName = "";
  File dir, root = SPIFFS.open("/");  
  while ((dir = root.openNextFile())) {
    fileName = String(dir.name());
    Serial.println(" " + fileName );
  }
}

void playWAV(const char* spiffsFile) {
  Serial.println("playWAV");

  // ffmpeg -i hapyou2.wav -acodec pcm_s16le -ar 44100 sound.wav
  // File file = SPIFFS.open("/sound.wav");  // 44100Hz, 16bit, linear PCM
  File file = SPIFFS.open(spiffsFile);
  file.seek(22);
  int ch = file.read();
  file.seek(offset);
  I2S_Init();
  while (file.readBytes(data, sizeof(data))) {
    if (ch == 2) I2S_Write(data, sizeof(data));
    else if (ch == 1) {
      for (int i = 0; i < sizeof(data); ++i) 
      {
        stereoData[4 * (i / 2) + i % 2] = data[i];
        stereoData[4 * (i / 2) + i % 2 + 2] = data[i];
      }
      I2S_Write(stereoData, sizeof(stereoData));
    }
  }
  file.close();
  for (int i = 0; i < sizeof(data); ++i) data[i] = 0; // to prevent buzzing
  for (int i = 0; i < 5; ++i) I2S_Write(data, sizeof(data));
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
  SPIFFS.begin();

  pinMode(LED1, OUTPUT);

  pixels.begin();
  pixels.clear();

  //ステータス初期化
  setupStatus();

  // WiFi 接続設定
  setupWiFi();

  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());

  // OSC受信設定
  OscWiFi.subscribe(bind_port, "/status/volume",
    [](const OscMessage& m)
    {
      float v = m.arg<float>(0);
      status.volume = v;
      Serial.printf("/status/volume %f\n", status.volume);
    }
  );

  // ファイル再生
  OscWiFi.subscribe(bind_port, "/status/play",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play %s\n", status.path.c_str());
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
  delay(500);
  Serial.println("loop");
  OscWiFi.update();

  if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY) == 0) {
    // TODO status.path の内容で 処理変更
    //SPIFFS mp3 ファイル 再生
    playMP3(status.path.c_str());
    //SPIFFS wav ファイル 再生
    // playWAV(status.path.c_str());
    // playWAV2(status.path.c_str());
    //URL ファイル 再生
    //TODO ダウンロード
    // playURL(status.path.c_str());
  } else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_COLOR) == 0) {
    // LED 色設定
    int rgb[3];
    rgbStr2rgbInt(status.color[0].c_str(),rgb);
    pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[1].c_str(),rgb);
    pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[2].c_str(),rgb);
    pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));

    rgbStr2rgbInt(status.color[3].c_str(),rgb);
    pixels.setPixelColor(0, pixels.Color(rgb[0], rgb[1], rgb[2]));
  }

  /*
  // dirSPIFFS();
  
  // playMP3();
  // playWAV();
  // playWAV2();

  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.setPixelColor(1, pixels.Color(150, 0, 0));
  pixels.show();
  delay(DELAYVAL);

  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.setPixelColor(1, pixels.Color(0, 150, 0));
  pixels.show();
  delay(DELAYVAL);

  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.setPixelColor(1, pixels.Color(0, 0, 150));
  pixels.show();
  delay(DELAYVAL);

  // pixels.clear();

  
  Serial.println("loop");
  digitalWrite(LED1, HIGH);
  delay(1000);
  digitalWrite(LED1, LOW);
  delay(1000);
  */
}
