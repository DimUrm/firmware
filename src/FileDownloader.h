#ifndef __FILE_DOWNLOADER_H__
#define __FILE_DOWNLOADER_H__

#include <HTTPClient.h>

// 
// https://github.com/tfuru/ESP8266Audio/blob/master/src/AudioFileSourceHTTPStream.h
// https://github.com/tfuru/ESP8266Audio/blob/master/src/AudioFileSourceHTTPStream.cpp
// https://github.com/tfuru/ESP8266Audio/blob/master/src/AudioFileSourceBuffer.cpp

class FileDownloader {
public:
    FileDownloader();
    ~FileDownloader();
    bool open(const char *url);
    uint32_t read(void *data, uint32_t len);
    uint32_t getSize();
    uint32_t getPos();
    bool close();

    char* getHash();
    char* getHashFileName(const char* ext);
private:
    uint32_t readInternal(void *data, uint32_t len, bool nonBlock);

    HTTPClient http;
    uint32_t size;
    uint32_t pos;
    char _url[256];
    char* _hash;
    char _hashFileName[32];

    int reconnectTries;
    int reconnectDelayMs;
};

#endif //__FILE_DOWNLOADER_H__