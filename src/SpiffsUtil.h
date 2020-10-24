#ifndef __SPIFFS_UTIL_H__
#define __SPIFFS_UTIL_H__

#include "FS.h"
#include <SPIFFS.h>

class SpiffsUtil {
public:
    SpiffsUtil();
    ~SpiffsUtil();
    void begin();
    File open(const char* fileName, const char* mode);
    size_t write(const uint8_t *buf, size_t size);
    size_t read(uint8_t* buf, size_t size);
    size_t position();
    size_t size();
    void close();
    bool exists(const char* path);
    void remove(const char* fileName);
    void listDir(const char * dirname, uint8_t levels);
private:
    File fw;
};

#endif //__SPIFFS_UTIL_H__