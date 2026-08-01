#include "buttons.h"


unsigned long okStart = 0;
unsigned long okCooldownUntil = 0;

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
    unsigned long now = millis();

    if(now - okLastTime > DEBOUNCE_MS)
    {
        okLastTime = now;

        if(now < okCooldownUntil)
        {
            okLast = ok;
            leftLast = left;
            rightLast = right;
            return;
        }

        unsigned long duration = now - okStart;

        if(duration > 1000)
            okLongEvent = true;
        else
        {
            okEvent = true;

            okCooldownUntil = now + 450;
        }
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
    if(okEvent)
    {
        okEvent = false;
        return true;
    }

    return false;
}



bool okLongPressed()
{
    if(okLongEvent)
    {
        okLongEvent = false;
        return true;
    }

    return false;
}



bool leftPressed()
{
    if(leftEvent)
    {
        leftEvent = false;
        return true;
    }

    return false;
}



bool rightPressed()
{
    if(rightEvent)
    {
        rightEvent = false;
        return true;
    }

    return false;
}