#include <Arduino.h>
#include "LedUtil.h"

LedUtil::LedUtil(): _pixels(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800){

}

LedUtil::~LedUtil(){
    
}

void LedUtil::setup(uint8_t pin){
  pinMode(pin, OUTPUT);    
  _pixels.begin();
  _pixels.clear();
}

void LedUtil::setPixelColor(uint16_t n, const char* src){
    rgbStr2rgbInt(src,_rgb);
    _pixels.setPixelColor(n, _pixels.Color(_rgb[0], _rgb[1], _rgb[2]));
}

void LedUtil::show() {
    _pixels.show();
}

// "r,g,b" を int[3] に変換する
void LedUtil::rgbStr2rgbInt(const char* src, int rgb[]) {
  char buf[20];
  strcpy(buf,src);
  rgb[0] = atoi(strtok(buf, ","));
  rgb[1] = atoi(strtok(NULL, ","));
  rgb[2] = atoi(strtok(NULL, ","));
}
