#include <Arduino.h>
#include <ArduinoOSC.h>

#include <FS.h>
#include <SPIFFS.h>

#include <SPI.h>

#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#include "FileDownloader.h"
#include "SpiffsUtil.h"

#include "I2S.h"
#include "LedUtil.h"
#include "WifiUtil.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>

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
#define DEVICE_STATUS_MODE_PLAY_STREAM_MP3    "play_stream_mp3"
#define DEVICE_STATUS_MODE_COLOR   "color"

AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *streamFile;
AudioFileSourceBuffer *buff;
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

// LED D2
#define D2 13

// D2 
void setStatusLED(bool isON) {
  digitalWrite(D2, (isON)? HIGH : LOW);
}

void setLedColors();
void stopMp3();
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
void StatusCallback(void *cbData, int code, const char *string);

// 拡張子で ContentType を切り替える
String getContentType(String filename){
  if(filename.endsWith(".html") || filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js"))  return "text/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else return "text/plain";
}

void webServerSetup() {
  Serial.println("webServerSetup");

  // API 実装 ---- 
  {
    // IPAddress 取得 API
    // curl -X POST -H "Content-Type: application/json" -d '{}' http://connecteddoll.local/api/ip    
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/ip", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);
      IPAddress localIP = WiFi.localIP();
      String output = "{\"status\":\"OK\",\"ip\":\"" + localIP.toString() + "\"}";
      request->send(200, "application/json", output);

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // SPIFFS DIR
    // curl -X POST -H "Content-Type: application/json" -d '{}' http://connecteddoll.local/api/dir
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/dir", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);

      status.mode = DEVICE_STATUS_MODE_DIR;
      // TODO dir 結果を jsonで返す
      String output = "{\"status\":\"OK\"}";
      request->send(200, "application/json", output);
      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // VOLUME 設定
    // curl -X POST -H "Content-Type: application/json" -d '{"volume":0.3}' http://connecteddoll.local/api/volume
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/volume", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);

      JsonObject& root = json.as<JsonObject>();
      const float volume = root.get<float>("volume");
      status.volume = volume;
      Serial.printf("volume %.2f\n", volume);

      String output = "{\"status\":\"OK\",\"volume\":" + String(volume) + "}";
      request->send(200, "application/json", output);

      // 音量設定
      out->SetGain(status.volume);  

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // LED 制御 API
    // curl -X POST -H "Content-Type: application/json" -d '{"leds": ["255,255,255","255,255,255","255,255,255","255,255,255"]}' http://connecteddoll.local/api/led
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/led", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);
      JsonObject& root = json.as<JsonObject>();
      const char* led0 = root["leds"][0];
      const char* led1 = root["leds"][1];
      const char* led2 = root["leds"][2];
      const char* led3 = root["leds"][3];

      status.color[0] = led0;
      status.color[1] = led1;
      status.color[2] = led2;
      status.color[3] = led3;
      // status.mode = DEVICE_STATUS_MODE_COLOR;
      setLedColors();

      String output = "{\"status\":\"OK\"}";
      request->send(200, "application/json", output);

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }
  
  {
    // mp3 再生 停止 API
    // curl -X POST -H "Content-Type: application/json" -d '{}' http://connecteddoll.local/api/stop/mp3    
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/stop/mp3", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);
      
      String output = "{\"status\":\"OK\"}";
      request->send(200, "application/json", output);
      
      stopMp3();
      
      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // ローカル mp3 再生 API
    // curl -X POST -H "Content-Type: application/json" -d '{"path": "/d3_IcaDhcDM.mp3"}' http://connecteddoll.local/api/play/mp3    
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/play/mp3", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);

      JsonObject& root = json.as<JsonObject>();
      const char* path = root["path"];

      status.path = path;
      status.mode = DEVICE_STATUS_MODE_PLAY_MP3;

      String output = "{\"status\":\"OK\",\"path\":\"" + String(status.path) + "\"}";
      request->send(200, "application/json", output);

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // URL mp3 ダウンロード再生 API
    // curl -X POST -H "Content-Type: application/json" -d '{"url": "https://file-examples-com.github.io/uploads/2017/11/file_example_MP3_700KB.mp3"}' http://connecteddoll.local/api/play/url/mp3  
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/play/url/mp3", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);

      JsonObject& root = json.as<JsonObject>();
      const char* url = root["url"];

      status.path = url;
      status.mode = DEVICE_STATUS_MODE_PLAY_URL_MP3;

      String output = "{\"status\":\"OK\",\"url\":\"" + String(status.path) + "\"}";
      request->send(200, "application/json", output);

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    // HTTPストリーミング mp3 再生 API
    // curl -X POST -H "Content-Type: application/json" -d '{"url": "http://35.202.184.164/api/stream/1WTy2yqKI4w"}' http://connecteddoll.local/api/play/stream/mp3
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/play/stream/mp3", [](AsyncWebServerRequest *request, JsonVariant &json) {
      setStatusLED(true);

      JsonObject& root = json.as<JsonObject>();
      const char* url = root["url"];

      status.path = url;
      status.mode = DEVICE_STATUS_MODE_PLAY_STREAM_MP3;

      String output = "{\"status\":\"OK\",\"url\":\"" + String(status.path) + "\"}";
      request->send(200, "application/json", output);

      setStatusLED(false);  
    });
    webServer.addHandler(handler);
  }

  {
    webServer.serveStatic("/", SPIFFS, "/www").setDefaultFile("index.html");
  }

  webServer.begin();
}

void audioInit() {
  Serial.println("audioInit");
  out = new AudioOutputI2S();
  out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  // out->SetChannels(1);
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
  Serial.printf("file path: %s\n", spiffsFile);
  Serial.print("file size:"); Serial.println( file->getSize() );
  Serial.printf("volume %.1f\n", status.volume);
  if (file->getSize() == 0) return;

  // 音量設定
  out->SetGain(status.volume);

  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  while(mp3->isRunning()){
    if (!mp3->loop()) mp3->stop();
  }
}

// URL指定されたMP3ファイルをダウンロードして再生する
void playUrlMp3(const char* url){
  Serial.println("playUrlMp3");
  // SPIFFS にファイルをダウンロードする
  const char* fireName = downloadFile(url);
  playMp3(fireName);
}

// http stream での MP3 再生
void playStreamMp3(const char* url){
  Serial.println("playStreamMp3");
  streamFile = new AudioFileSourceICYStream(url);
  streamFile->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(streamFile, 2048);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");

  // 音量設定
  out->SetGain(status.volume);

  mp3 = new AudioGeneratorMP3();
  mp3->begin(streamFile, out);
  while(mp3->isRunning()){
    if (!mp3->loop()) mp3->stop();
  }
}

// 再生停止
void stopMp3(){
  if (mp3 == nullptr) return;
  if (mp3->isRunning()) {
    mp3->stop();
  }
}

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2)-1]=0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1)-1]=0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

//ステータス初期化
void setupStatus(){
  status.mode = DEVICE_STATUS_MODE_DEFAULT;
  status.volume = 0.1f;
}

// OSC 初期化
void oscWiFiSetup() {
  Serial.println("oscWiFiSetup");
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
      
      // 音量設定
      out->SetGain(status.volume);      
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

  // HTTPストリーミング MP3ファイル再生
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/play/stream/mp3",
    [](const OscMessage& m)
    {
      status.mode = DEVICE_STATUS_MODE_PLAY_STREAM_MP3;
      status.path = m.arg<String>(0);
      Serial.printf("/status/play/stream/mp3 %s\n", status.path.c_str());
    }
  );

  // MP3ファイル再生 停止
  OscWiFi.subscribe(OSC_BIND_PORT, "/status/stop/mp3",
    [](const OscMessage& m)
    {
      stopMp3();
      Serial.printf("/status/stop/mp3\n");
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
      
      setLedColors();
    }
  );
}

// LED 色設定
void setLedColors(){
  ledUtil.setPixelColor(0, status.color[0].c_str());
  ledUtil.setPixelColor(1, status.color[1].c_str());
  ledUtil.setPixelColor(2, status.color[2].c_str());
  ledUtil.setPixelColor(3, status.color[3].c_str());
  ledUtil.show();
  status.mode = DEVICE_STATUS_MODE_DEFAULT;
}

void setup() {
  Serial.begin(115200);
  
  pinMode(D2, OUTPUT);
  digitalWrite(D2, LOW);

  // SPIFFS 初期化
  spiffsUtil.begin();

  // LED 初期化
  ledUtil.setup(LED1);

  //ステータス初期化
  setupStatus();

  // mDNS 設定
  wifiUtil.setupMDNS(MDNS_HOST, HTTP_PORT, OSC_BIND_PORT);
  
  // WiFi 接続設定
  wifiUtil.setupWiFi(WIFI_SSID);
  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());

  //　オーディオ 初期化
  audioInit();
  
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
    setStatusLED(true);
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    spiffsUtil.listDir("/", 0);
    setStatusLED(false);
  }
  else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY_MP3) == 0) {
    setStatusLED(true);
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    //SPIFFS mp3 ファイル 再生
    playMp3(status.path.c_str());
    setStatusLED(false);
  }
  else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY_URL_MP3) == 0) {
    setStatusLED(true);
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    //URL ファイル 再生 ダウンロードし再生する
    playUrlMp3(status.path.c_str());
    setStatusLED(false);
  } else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_PLAY_STREAM_MP3) == 0) {
    setStatusLED(true);
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    //HTTPストリーミング で ファイル 再生する
    playStreamMp3(status.path.c_str());
    setStatusLED(false);    
  } else if ( strcmp(status.mode.c_str(),DEVICE_STATUS_MODE_COLOR) == 0) {
    setStatusLED(true);
    status.mode = DEVICE_STATUS_MODE_DEFAULT;
    // LED 色設定
    setLedColors();
    setStatusLED(false);
  }
}

void loop() {
  // Serial.println("loop");
  // delay(200);
  loopOscCmd();
}
