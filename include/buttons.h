#ifndef BUTTONS_H
#define BUTTONS_H

#include <Arduino.h>

#define BTN_OK     6
#define BTN_LEFT   7
#define BTN_RIGHT  3


void buttonsInit();


bool okPressed();
bool okLongPressed();

bool leftPressed();
bool rightPressed();


#endif