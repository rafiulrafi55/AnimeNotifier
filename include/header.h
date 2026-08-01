#ifndef HEADER_H
#define HEADER_H

#include <Arduino.h>
#include <U8g2lib.h>

void drawHeader(U8G2 &display, int batteryLevel, bool lowPowerModeOn, const char *timeText, const char *ssid);

#endif