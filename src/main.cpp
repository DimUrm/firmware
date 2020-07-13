#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "SPI.h"
#include "I2S.h"
#include "FS.h"
#include "SPIFFS.h"

#include <WiFi.h>
#include <WiFiClient.h>

#include <MQTT.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#define LED1 13

#define RGB_LED 4
#define NUMPIXELS 2
Adafruit_NeoPixel pixels(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

const int offset = 0x2C;
char data[800];
char stereoData[1600];

void playWAV() {
  Serial.println("playWAV");
  SPIFFS.begin();

  // ffmpeg -i hapyou2.wav -acodec pcm_s16le -ar 44100 sound.wav
  File file = SPIFFS.open("/sound.wav");  // 44100Hz, 16bit, linear PCM
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

void setup() {
  Serial.begin(115200);
  
  playWAV();
  
  pinMode(LED1, OUTPUT);

  pixels.begin();
  pixels.clear();
}

void loop() {

  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(150, 0, 0));
    pixels.show();
    delay(DELAYVAL);

    pixels.setPixelColor(i, pixels.Color(0, 150, 0));
    pixels.show();
    delay(DELAYVAL);

    pixels.setPixelColor(i, pixels.Color(0, 0, 150));
    pixels.show();
    delay(DELAYVAL);

    pixels.setPixelColor(i, pixels.Color(150, 150, 150));
    pixels.show();
    delay(DELAYVAL);
  }
  pixels.clear();

  Serial.println("loop");
  digitalWrite(LED1, HIGH);
  delay(1000);
  digitalWrite(LED1, LOW);
  delay(1000);
}
