#include <Arduino.h>
#include <Wire.h>

#include "display.h"
#include "pins.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE
);

void displayInit()
{
    Wire.begin(OLED_SDA, OLED_SCL);

    display.begin();

    display.clearBuffer();
    display.setFont(u8g2_font_4x6_tr);
    display.drawStr(20, 32, "AnimeNotify");
    display.sendBuffer();
}