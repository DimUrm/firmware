#ifndef __WIFI_UTIL_H__
#define __WIFI_UTIL_H__

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>

// WiFi 設定
class WifiUtil {
public:
    WifiUtil();
    ~WifiUtil();
    void setupWiFi(const char* name);
    void resetSettings();
    static void configModeCallback(WiFiManager *myWiFiManager);

    void setupMDNS(const char* name, uint16_t httpPort,uint16_t oscPort);
private:
    WiFiManager _wifiManager;
};

#endif //__WIFI_UTIL_H__