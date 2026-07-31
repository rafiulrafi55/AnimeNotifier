#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

void wifiInit();

bool wifiConnect(const String &ssid, const String &password);

bool wifiConnected();

String wifiSSID();

#endif