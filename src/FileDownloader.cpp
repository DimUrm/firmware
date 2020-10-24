#include "FileDownloader.h"
#include <MD5.h>

FileDownloader::FileDownloader() {
  pos = 0;
  reconnectTries = 0;
  _url[0] = 0;
}

FileDownloader::~FileDownloader() {
    close();
}

bool FileDownloader::open(const char *url){
    pos = 0;
    http.begin(url);
    http.setReuse(true);
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
      http.end();
      // cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
      return false;
    }
    size = http.getSize();

    strncpy(_url, url, sizeof(_url));
    _url[sizeof(_url)-1] = 0;

    _hash = MD5::make_digest(MD5::make_hash(_url), 16);

    return true;
}

uint32_t FileDownloader::readInternal(void *data, uint32_t len, bool nonBlock){
retry:
  if (!http.connected()) {
    //cb.st(STATUS_DISCONNECTED, PSTR("Stream disconnected"));
    http.end();
    for (int i = 0; i < reconnectTries; i++) {
      // char buff[32];
      // sprintf_P(buff, PSTR("Attempting to reconnect, try %d"), i);
      // cb.st(STATUS_RECONNECTING, buff);
      delay(reconnectDelayMs);
      if (open(_url)) {
        // cb.st(STATUS_RECONNECTED, PSTR("Stream reconnected"));
        break;
      }
    }
    if (!http.connected()) {
      // cb.st(STATUS_DISCONNECTED, PSTR("Unable to reconnect"));
      return 0;
    }
  }
  if ((size > 0) && (pos >= size)) return 0;

  WiFiClient *stream = http.getStreamPtr();

  // Can't read past EOF...
  if ( (size > 0) && (len > (uint32_t)(pos - size)) ) len = pos - size;

  if (!nonBlock) {
    int start = millis();
    while ((stream->available() < (int)len) && (millis() - start < 500)) yield();
  }

  size_t avail = stream->available();
  if (!nonBlock && !avail) {
    // cb.st(STATUS_NODATA, PSTR("No stream data available"));
    http.end();
    goto retry;
  }
  if (avail == 0) return 0;
  if (avail < len) len = avail;

  int read = stream->read(reinterpret_cast<uint8_t*>(data), len);
  pos += read;
  return read;
}

uint32_t FileDownloader::read(void *data, uint32_t len){
    return readInternal(data, len, false);
}

uint32_t FileDownloader::getSize(){
    return size;
}

uint32_t FileDownloader::getPos() {
  return pos;
}

char* FileDownloader::getHash() {
  return _hash;
}

// URLをハッシュ化したファイル名
char* FileDownloader::getHashFileName(const char* ext) {
  memset(_hashFileName, '\0', sizeof(_hashFileName));
  sprintf(_hashFileName, "/%.17s.%s", getHash(), ext);
  return _hashFileName;
}

bool FileDownloader::close(){
    http.end();
    return true;
}