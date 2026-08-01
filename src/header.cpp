#include "header.h"

void drawHeader(U8G2 &display, int batteryLevel, const char *timeText, const char *ssid)
{
    if (batteryLevel > 100)
        batteryLevel = 100;

    if (batteryLevel < 0)
        batteryLevel = 0;

    display.drawFrame(2, 3, 18, 9);
    display.drawBox(20, 5, 2, 5);

    int fillWidth = map(batteryLevel, 0, 100, 0, 14);
    if(fillWidth > 0)
    {
        display.drawBox(4, 5, fillWidth, 5);
    }

    display.setFont(u8g2_font_5x7_tr);

    const char *shownTime = (timeText != nullptr && strlen(timeText) > 0) ? timeText : "--:--";
    display.drawStr(26, 10, shownTime);


    // WiFi text
    display.drawStr(62, 10, "WiFi:");

    if(ssid != nullptr && strlen(ssid) > 0)
    {
        display.drawStr(89, 10, ssid);
    }
}