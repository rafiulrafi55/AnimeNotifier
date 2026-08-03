#include "time_manager.h"
#include <WiFi.h>
#include <time.h>


const char* ntpServer = "pool.ntp.org";


// Bangladesh timezone UTC+6
const long gmtOffset_sec = 21600;

const int daylightOffset_sec = 0;


bool synced = false;

void syncTime()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    configTime(
        gmtOffset_sec,
        daylightOffset_sec,
        ntpServer
    );

    struct tm timeinfo;
    getLocalTime(&timeinfo, 5000);
}

void initTime()
{
    configTime(
        gmtOffset_sec,
        daylightOffset_sec,
        ntpServer
    );


    setenv("TZ","Asia/Dhaka",1);
    tzset();


    struct tm timeinfo;


    if(getLocalTime(&timeinfo))
    {
        synced = true;
    }
}


bool timeReady()
{
    return synced;
}



String currentDate()
{
    struct tm timeinfo;


    if(!getLocalTime(&timeinfo))
        return "";


    char buffer[11];


    sprintf(
        buffer,
        "%04d-%02d-%02d",
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday
    );


    return String(buffer);
}