#include "buttons.h"


unsigned long okStart = 0;

bool okLast = HIGH;
bool leftLast = HIGH;
bool rightLast = HIGH;


bool okEvent = false;
bool okLongEvent = false;

bool leftEvent = false;
bool rightEvent = false;
unsigned long okLastTime = 0;
const unsigned long DEBOUNCE_MS = 200;


void buttonsInit()
{
    pinMode(BTN_OK, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
}



void buttonsUpdate()
{
    bool ok = digitalRead(BTN_OK);
    bool left = digitalRead(BTN_LEFT);
    bool right = digitalRead(BTN_RIGHT);



    // OK press
    if(ok == LOW && okLast == HIGH)
    {
        okStart = millis();
    }


    if(ok == HIGH && okLast == LOW)
{
    if(millis() - okLastTime > DEBOUNCE_MS)
    {
        okLastTime = millis();

        unsigned long duration = millis() - okStart;

        if(duration > 1000)
            okLongEvent = true;
        else
            okEvent = true;
    }
}


    // LEFT
    if(left == LOW && leftLast == HIGH)
    {
        leftEvent = true;
    }



    // RIGHT
    if(right == LOW && rightLast == HIGH)
    {
        rightEvent = true;
    }



    okLast = ok;
    leftLast = left;
    rightLast = right;
}




bool okPressed()
{
    buttonsUpdate();

    if(okEvent)
    {
        okEvent = false;
        return true;
    }

    return false;
}



bool okLongPressed()
{
    buttonsUpdate();

    if(okLongEvent)
    {
        okLongEvent = false;
        return true;
    }

    return false;
}



bool leftPressed()
{
    buttonsUpdate();

    if(leftEvent)
    {
        leftEvent = false;
        return true;
    }

    return false;
}



bool rightPressed()
{
    buttonsUpdate();

    if(rightEvent)
    {
        rightEvent = false;
        return true;
    }

    return false;
}