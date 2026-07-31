#include <Arduino.h>

#include "battery.h"

#define BATTERY_PIN 0
#define ADC_CAL     0.899f

void batteryInit()
{
    analogReadResolution(12);
}

float batteryVoltage()
{
    uint32_t sum = 0;

    for(int i = 0; i < 64; i++)
    {
        sum += analogRead(BATTERY_PIN);
        delay(1);
    }

    float raw = sum / 64.0f;

    float adc = raw * (3.3f / 4095.0f);

    return adc * 2.0f * ADC_CAL;
}

int batteryPercent()
{
    float v = batteryVoltage();

    if(v >= 4.20) return 100;
    if(v >= 4.15) return 95;
    if(v >= 4.10) return 90;
    if(v >= 4.05) return 85;
    if(v >= 4.00) return 80;
    if(v >= 3.95) return 75;
    if(v >= 3.90) return 70;
    if(v >= 3.85) return 65;
    if(v >= 3.80) return 60;
    if(v >= 3.75) return 55;
    if(v >= 3.70) return 50;
    if(v >= 3.65) return 45;
    if(v >= 3.60) return 40;
    if(v >= 3.55) return 35;
    if(v >= 3.50) return 30;
    if(v >= 3.45) return 25;
    if(v >= 3.40) return 20;
    if(v >= 3.35) return 15;
    if(v >= 3.30) return 10;
    if(v >= 3.20) return 5;

    return 0;
}