#include <Arduino.h>
#include <WiFi.h>
#include "display.h"
#include "header.h"
#include "battery.h"
#include "buttons.h"
#include "wifi_manager.h"
#include <Preferences.h>
#include "tmdb.h"
#include "time_manager.h"
#include "anime.h"
#include "anilist.h"


#define BUZZER 21

bool wifiScanned = false;
String currentSSID = "";
Preferences prefs;
unsigned long wifiConnectStart = 0;
bool wifiConnecting = false;
bool wifiSaved = false;
unsigned long movieUpdateTimer = 0;
unsigned long animeUpdateTimer = 0;

bool moviesLoaded = false;
bool animeLoaded = false;

const unsigned long UPDATE_INTERVAL = 900000; 
// 15 minutes

enum Screen
{
    SCREEN_HOME,
    SCREEN_MENU,

    SCREEN_WIFI_MENU,
    SCREEN_WIFI_SCAN,
    SCREEN_WIFI_KEYBOARD,
    SCREEN_WIFI_PASSWORD,
    SCREEN_KEYBOARD_MENU,
    SCREEN_WIFI_CONNECTING,

    SCREEN_SETTINGS,
    SCREEN_ABOUT
};

enum MainMenuItem
{
    MAIN_MENU_WIFI,
    MAIN_MENU_SETTINGS,
    MAIN_MENU_ABOUT
};

Screen currentScreen = SCREEN_HOME;
MainMenuItem selectedMenu = MAIN_MENU_WIFI;

enum WifiMenuItem
{
    WIFI_OPTION_SCAN,
    WIFI_OPTION_MANUAL
};

enum KeyboardMenuItem
{
    KEY_DONE,
    KEY_DELETE,
    KEY_SPACE,
    KEY_MODE,
    KEY_CLEAR,
    KEY_CANCEL
};

KeyboardMenuItem keyboardMenu = KEY_DONE;

WifiMenuItem wifiMenu = WIFI_OPTION_SCAN;


String networks[20];

int networkCount = 0;
int selectedNetwork = 0;
int networkTop = 0;

String selectedSSID = "";
String password = "";

const char lowerChars[] =
"abcdefghijklmnopqrstuvwxyz";
const char upperChars[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const char numberChars[] =
"0123456789";

const char symbolChars[] =
"!@#$%^&*()-_=+.,:/?";

int charIndex = 0;

enum KeyboardMode
{
    MODE_LOWER,
    MODE_UPPER,
    MODE_NUMBER,
    MODE_SYMBOL
};

KeyboardMode keyboardMode = MODE_LOWER;

const char* getCurrentCharset()
{
    switch(keyboardMode)
    {
        case MODE_LOWER:
            return lowerChars;

        case MODE_UPPER:
            return upperChars;

        case MODE_NUMBER:
            return numberChars;

        case MODE_SYMBOL:
            return symbolChars;
    }

    return lowerChars;
}

void nextKeyboardMode()
{
    switch(keyboardMode)
    {
        case MODE_LOWER:
            keyboardMode = MODE_UPPER;
            break;

        case MODE_UPPER:
            keyboardMode = MODE_NUMBER;
            break;

        case MODE_NUMBER:
            keyboardMode = MODE_SYMBOL;
            break;

        case MODE_SYMBOL:
            keyboardMode = MODE_LOWER;
            break;
    }

    // Always start at the first character
    charIndex = 0;
}

void scanNetworks()
{
    display.clearBuffer();
    drawHeader(display, batteryPercent(), "");

    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(20, 30, "Scanning...");
    display.sendBuffer();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    delay(100);

    networkCount = WiFi.scanNetworks();

    for(int i = 0; i < networkCount && i < 20; i++)
    {
        networks[i] = WiFi.SSID(i);
    }
}


void setup()
{
  wifiInit();
    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    displayInit();

    batteryInit();

    buttonsInit();

    prefs.begin("wifi", true);

String savedSSID = prefs.getString("ssid", "");
String savedPass = prefs.getString("pass", "");

prefs.end();

if(savedSSID.length())
{
    WiFi.scanDelete();

    WiFi.mode(WIFI_STA);

    delay(100);

    WiFi.begin(
        savedSSID.c_str(),
        savedPass.c_str()
    );
}
}


void loop()
{
    display.clearBuffer();
    if(WiFi.status() == WL_CONNECTED)
{
    currentSSID = WiFi.SSID();
}

    drawHeader(display, batteryPercent(), currentSSID.c_str());
    // Universal Back / Menu
if(okLongPressed())
{
    switch(currentScreen)
    {
        case SCREEN_HOME:
            currentScreen = SCREEN_MENU;
            break;

        case SCREEN_MENU:
            currentScreen = SCREEN_HOME;
            break;

        case SCREEN_WIFI_PASSWORD:
            keyboardMenu = KEY_DONE;
            currentScreen = SCREEN_KEYBOARD_MENU;
            break;

        case SCREEN_KEYBOARD_MENU:
            currentScreen = SCREEN_WIFI_PASSWORD;
            break;

        default:
            currentScreen = SCREEN_MENU;
            break;
    }
}



    // Draw current screen
if(currentScreen == SCREEN_HOME)
{
    display.setFont(u8g2_font_4x6_tr);


    for(int i=0;i<3;i++)
    {
        int y = 20 + i*14;


        // Anime left
display.drawStr(
    0,
    y,
    animeList[i].title.c_str()
);

display.drawStr(
    0,
    y+6,
    animeList[i].time.c_str()
);


        // Movie title
        display.drawStr(
            65,
            y,
            movies[i].title.c_str()
        );


        // Date
        display.drawStr(
            65,
            y+6,
            movies[i].date.c_str()
        );
    }
}


   if(currentScreen == SCREEN_MENU)
{
    // Navigation
    if(leftPressed())
    {
        if(selectedMenu == MAIN_MENU_WIFI)
            selectedMenu = MAIN_MENU_ABOUT;
        else
            selectedMenu = (MainMenuItem)(selectedMenu - 1);
    }

    if(rightPressed())
    {
        if(selectedMenu == MAIN_MENU_ABOUT)
            selectedMenu = MAIN_MENU_WIFI;
        else
            selectedMenu = (MainMenuItem)(selectedMenu + 1);
    }

    // Open selected item
    if(okPressed())
    {
        switch(selectedMenu)
        {
            case MAIN_MENU_WIFI:
                currentScreen = SCREEN_WIFI_MENU;
                break;

            case MAIN_MENU_SETTINGS:
                currentScreen = SCREEN_SETTINGS;
                break;

            case MAIN_MENU_ABOUT:
                currentScreen = SCREEN_ABOUT;
                break;
        }
    }

    // Return Home
   

    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(
        10,
        24,
        selectedMenu == MAIN_MENU_WIFI ? "> WiFi" : "  WiFi"
    );

    display.drawStr(
        10,
        36,
        selectedMenu == MAIN_MENU_SETTINGS ? "> Settings" : "  Settings"
    );

    display.drawStr(
        10,
        48,
        selectedMenu == MAIN_MENU_ABOUT ? "> About" : "  About"
    );
}

if(currentScreen == SCREEN_WIFI_MENU)
{
    if(leftPressed())
    {
        wifiMenu = (wifiMenu == WIFI_OPTION_SCAN) ? WIFI_OPTION_MANUAL : WIFI_OPTION_SCAN;
    }

    if(rightPressed())
    {
        wifiMenu = (wifiMenu == WIFI_OPTION_MANUAL) ? WIFI_OPTION_SCAN : WIFI_OPTION_MANUAL;
    }

    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(
        10,
        24,
        wifiMenu == WIFI_OPTION_SCAN ? "> Scan Networks" : "  Scan Networks"
    );

    display.drawStr(
        10,
        38,
        wifiMenu == WIFI_OPTION_MANUAL ? "> Manual Entry" : "  Manual Entry"
    );

    if(okPressed())
    {
        switch(wifiMenu)
        {
            case WIFI_OPTION_SCAN:
    wifiScanned = false;

selectedNetwork = 0;
networkTop = 0;

currentScreen = SCREEN_WIFI_SCAN;
    break;

case WIFI_OPTION_MANUAL:
    currentScreen = SCREEN_WIFI_KEYBOARD;
    break;
        }
    }
}

if(currentScreen == SCREEN_WIFI_SCAN)
{
    // Scan once
    if(!wifiScanned)
    {
        scanNetworks();
        wifiScanned = true;
    }

    // Navigation
    if(leftPressed())
    {
        if(selectedNetwork > 0)
        {
            selectedNetwork--;

            if(selectedNetwork < networkTop)
                networkTop--;
        }
    }

    if(rightPressed())
    {
        if(selectedNetwork < networkCount - 1)
        {
            selectedNetwork++;

            if(selectedNetwork > networkTop + 3)
                networkTop++;
        }
    }

    // Select network
    if(okPressed())
{
    if(networkCount > 0)
    {
        selectedSSID = networks[selectedNetwork];

        password = "";
        charIndex = 0;
        keyboardMode = MODE_LOWER;

        currentScreen = SCREEN_WIFI_PASSWORD;
    }
}

    display.setFont(u8g2_font_5x7_tr);

    if(networkCount == 0)
    {
        display.drawStr(18, 34, "No Networks Found");
    }
    else
    {
        // Show 4 visible networks
        for(int i = 0; i < 4; i++)
        {
            int index = networkTop + i;

            if(index >= networkCount)
                break;

            String line =
                (index == selectedNetwork ? "> " : "  ") +
                networks[index];

            display.drawStr(5, 22 + (i * 10), line.c_str());
        }
    }
}

if(currentScreen == SCREEN_WIFI_KEYBOARD)
{
    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(10, 28, "SSID Input");
    display.drawStr(10, 42, "(Coming Soon)");
}
if(currentScreen == SCREEN_WIFI_PASSWORD)
{
  const char* charset = getCurrentCharset();
int charsetLength = strlen(charset);
    // Move character selection
    if(leftPressed())
    {
       charIndex--;

if(charIndex < 0)
    charIndex = charsetLength - 1;
    }

    if(rightPressed())
    {
        charIndex++;

if(charIndex >= charsetLength)
    charIndex = 0;
    }

    // Add selected character
    if(okPressed())
    {
        if(password.length() < 32)
            password += charset[charIndex];
    }


    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(0,18,"WiFi:");
    display.drawStr(35,18,selectedSSID.c_str());

    display.drawStr(0,32,"Password:");

    display.drawStr(0,44,password.c_str());

    char current[2];

    current[0] = charset[charIndex];
    current[1] = '\0';

    display.drawStr(54,60,current);
}

if(currentScreen == SCREEN_KEYBOARD_MENU)
{
    if(leftPressed())
    {
        if(keyboardMenu == KEY_DONE)
            keyboardMenu = KEY_CANCEL;
        else
            keyboardMenu = (KeyboardMenuItem)(keyboardMenu - 1);
    }

    if(rightPressed())
    {
        if(keyboardMenu == KEY_CANCEL)
            keyboardMenu = KEY_DONE;
        else
            keyboardMenu = (KeyboardMenuItem)(keyboardMenu + 1);
    }

    if(okPressed())
    {
        switch(keyboardMenu)
        {
            case KEY_DONE:

    wifiConnectStart = millis();
wifiConnecting = true;
wifiSaved = false;

wifiConnect(selectedSSID, password);

currentScreen = SCREEN_WIFI_CONNECTING;

    break;

            case KEY_DELETE:
                if(password.length() > 0)
                    password.remove(password.length() - 1);

                currentScreen = SCREEN_WIFI_PASSWORD;
                break;

            case KEY_SPACE:
                password += ' ';
                currentScreen = SCREEN_WIFI_PASSWORD;
                break;

            case KEY_MODE:
    nextKeyboardMode();
    currentScreen = SCREEN_WIFI_PASSWORD;
    break;

            case KEY_CLEAR:
                password = "";
                currentScreen = SCREEN_WIFI_PASSWORD;
                break;

            case KEY_CANCEL:
                currentScreen = SCREEN_WIFI_PASSWORD;
                break;
        }
    }

    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(
        5,18,
        keyboardMenu==KEY_DONE ?
        "> Done":"  Done");

    display.drawStr(
        5,28,
        keyboardMenu==KEY_DELETE ?
        "> Delete":"  Delete");

    display.drawStr(
        5,38,
        keyboardMenu==KEY_SPACE ?
        "> Space":"  Space");

   String modeText;

switch(keyboardMode)
{
    case MODE_LOWER:
        modeText = "abc";
        break;

    case MODE_UPPER:
        modeText = "ABC";
        break;

    case MODE_NUMBER:
        modeText = "123";
        break;

    case MODE_SYMBOL:
        modeText = "#+=";
        break;
}

String modeLine =
    (keyboardMenu == KEY_MODE ? "> " : "  ") +
    modeText;

display.drawStr(5, 48, modeLine.c_str());

    display.drawStr(
        68,18,
        keyboardMenu==KEY_CLEAR ?
        "> Clear":"  Clear");

    display.drawStr(
        68,28,
        keyboardMenu==KEY_CANCEL ?
        "> Cancel":"  Cancel");
}



if(currentScreen == SCREEN_WIFI_CONNECTING)
{
    display.setFont(u8g2_font_5x7_tr);

    if(WiFi.status() == WL_CONNECTED)
    {
        display.drawStr(20,25,"Connected!");

        if(!wifiSaved)
        {
            prefs.begin("wifi", false);

            prefs.putString("ssid", selectedSSID);
            prefs.putString("pass", password);

            prefs.end();

            currentSSID = selectedSSID;

            wifiSaved = true;
        }


        delay(1000);

    }
    else
    {
        display.drawStr(20,25,"Connecting...");
    }
}
if(WiFi.status() == WL_CONNECTED)
{
    if(!timeReady())
{
    initTime();
    setenv("TZ","Asia/Dhaka",1);
tzset();

    delay(100);
}
    unsigned long now = millis();


if(now - movieUpdateTimer > UPDATE_INTERVAL || !moviesLoaded)
{
    fetchMovies();
    movieUpdateTimer = now;
    moviesLoaded = true;
}


if(now - animeUpdateTimer > UPDATE_INTERVAL || !animeLoaded)
{
    fetchAnime();

animeUpdateTimer = now;

animeLoaded = true;

delay(500);
}

    currentScreen = SCREEN_HOME;
}




if(currentScreen == SCREEN_SETTINGS)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(25,35,"Settings");
}

if(currentScreen == SCREEN_ABOUT)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(35,35,"About");
}


    display.sendBuffer();

    delay(10);
}