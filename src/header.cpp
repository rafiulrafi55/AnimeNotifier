#include "header.h"

void drawHeader(U8G2 &display, int batteryLevel, const char *ssid)
{
    if (batteryLevel > 100)
        batteryLevel = 100;

    if (batteryLevel < 0)
        batteryLevel = 0;


    // Battery icon
    display.drawFrame(2, 3, 18, 9);
    display.drawBox(20, 5, 2, 5);

    int fillWidth = map(batteryLevel, 0, 100, 0, 14);

    if(fillWidth > 0)
    {
        display.drawBox(4, 5, fillWidth, 5);
    }


    // WiFi text
    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(70, 10, "WiFi:");

    if(ssid != nullptr && strlen(ssid) > 0)
    {
        display.drawStr(97, 10, ssid);
    }
}