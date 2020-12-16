#ifndef __LED_UTIL_H__
#define __LED_UTIL_H__

#include <Adafruit_NeoPixel.h>

#define LED1 13

#define RGB_LED 4
#define NUMPIXELS 4

class LedUtil {
public:
    LedUtil();
    ~LedUtil();
    void setup(uint8_t pin);
    void setPixelColor(uint16_t n, const char* src);
    void show();
private:
    void rgbStr2rgbInt(const char* src, int rgb[]);
private:
    Adafruit_NeoPixel _pixels;
    int _rgb[3];
};

#endif //__LED_UTIL_H__