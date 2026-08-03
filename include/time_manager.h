#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

void initTime();
bool timeReady();
void syncTime();
String currentDate();

#endif