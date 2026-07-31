#include "wifi_manager.h"

#include <WiFi.h>

void wifiInit()
{
    WiFi.disconnect(true);
    delay(100);

    WiFi.mode(WIFI_STA);

    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setAutoReconnect(true);

    delay(500);
}

bool wifiConnect(const String &ssid, const String &password)
{
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();

    while (millis() - start < 20000)
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        delay(100);
    }

    return false;
}

bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

String wifiSSID()
{
    if (WiFi.status() == WL_CONNECTED)
        return WiFi.SSID();

    return "";
}