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
bool wifiHasSavedCredentials = false;
bool wifiConnectionFailed = false;
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
    SCREEN_SETTINGS_TITLE_MENU,
    SCREEN_SETTINGS_MOVIES_COMING,
    SCREEN_SETTINGS_ANIME_LIST,
    SCREEN_SETTINGS_ANIME_ACTION_MENU,
    SCREEN_SETTINGS_DONE,
    SCREEN_ABOUT
};

enum MainMenuItem
{
    MAIN_MENU_WIFI,
    MAIN_MENU_SETTINGS,
    MAIN_MENU_ABOUT
};

enum SettingsMenuItem
{
    SETTINGS_DEFAULT,
    SETTINGS_SELECT_TITLE
};

enum TitleSourceItem
{
    TITLE_SOURCE_MOVIES,
    TITLE_SOURCE_ANIME
};

enum AnimeActionItem
{
    ANIME_ACTION_DONE,
    ANIME_ACTION_CANCEL
};

Screen currentScreen = SCREEN_HOME;
MainMenuItem selectedMenu = MAIN_MENU_WIFI;
SettingsMenuItem selectedSettingsMenu = SETTINGS_DEFAULT;
TitleSourceItem selectedTitleSource = TITLE_SOURCE_MOVIES;
AnimeActionItem selectedAnimeAction = ANIME_ACTION_DONE;

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

AnimeChoice seasonAnimeChoices[MAX_ANIME_CHOICES];
int seasonAnimeCount = 0;
String selectedAnimeTitles[MAX_ANIME_CHOICES];
int selectedAnimeTitleCount = 0;
int seasonAnimeCursor = 0;
int seasonAnimeTop = 0;
unsigned long selectionDoneUntil = 0;
const int VISIBLE_ANIME_CHOICES = 7;

bool animeTitleIsSelected(const String &title)
{
    for(int i = 0; i < selectedAnimeTitleCount; i++)
    {
        if(selectedAnimeTitles[i] == title)
            return true;
    }

    return false;
}

void loadSelectedAnimeTitles()
{
    selectedAnimeTitleCount = 0;

    prefs.begin("anime", true);
    String stored = prefs.getString("titles", "");
    prefs.end();

    while(stored.length() > 0 && selectedAnimeTitleCount < MAX_ANIME_CHOICES)
    {
        int separator = stored.indexOf('\n');
        String entry = separator >= 0 ? stored.substring(0, separator) : stored;
        entry.trim();

        if(entry.length() > 0)
            selectedAnimeTitles[selectedAnimeTitleCount++] = entry;

        if(separator < 0)
            break;

        stored = stored.substring(separator + 1);
    }
}

void saveSelectedAnimeTitles()
{
    String stored;

    for(int i = 0; i < selectedAnimeTitleCount; i++)
    {
        if(i > 0)
            stored += '\n';

        stored += selectedAnimeTitles[i];
    }

    prefs.begin("anime", false);
    prefs.putString("titles", stored);
    prefs.end();
}

void clearSelectedAnimeTitles()
{
    selectedAnimeTitleCount = 0;
    prefs.begin("anime", false);
    prefs.putString("titles", "");
    prefs.end();
}

void syncSelectedAnimeChoices()
{
    for(int i = 0; i < seasonAnimeCount; i++)
        seasonAnimeChoices[i].selected = animeTitleIsSelected(seasonAnimeChoices[i].title);
}

void rebuildSelectedAnimeTitlesFromChoices()
{
    selectedAnimeTitleCount = 0;

    for(int i = 0; i < seasonAnimeCount; i++)
    {
        if(!seasonAnimeChoices[i].selected)
            continue;

        if(selectedAnimeTitleCount >= MAX_ANIME_CHOICES)
            break;

        selectedAnimeTitles[selectedAnimeTitleCount++] = seasonAnimeChoices[i].title;
    }
}

String trimDisplayText(const String &text, size_t maxChars)
{
    if (text.length() <= maxChars)
        return text;

    return text.substring(0, maxChars);
}

void drawScrollingText(U8G2 &oled, int x, int y, int width, const String &text, unsigned long now)
{
    if (text.length() == 0)
        return;

    int textWidth = oled.getStrWidth(text.c_str());

    if (textWidth <= width)
    {
        oled.drawStr(x, y, text.c_str());
        return;
    }

    const int gap = 16;
    const unsigned long frameMs = 35;
    int cycle = textWidth + gap;
    int offset = (now / frameMs) % cycle;
    int drawX = x + width - offset;

    oled.setClipWindow(x, y - 6, x + width - 1, y + 1);
    oled.drawStr(drawX, y, text.c_str());
    oled.setClipWindow(0, 0, 127, 63);
}

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
    wifiHasSavedCredentials = true;
    wifiConnecting = true;
    wifiConnectionFailed = false;
    wifiConnectStart = millis();

    WiFi.scanDelete();

    WiFi.mode(WIFI_STA);

    delay(100);

    WiFi.begin(
        savedSSID.c_str(),
        savedPass.c_str()
    );
}
else
{
    wifiHasSavedCredentials = false;
    wifiConnecting = false;
    wifiConnectionFailed = false;
}

loadSelectedAnimeTitles();
}


void loop()
{
    buttonsUpdate();

    bool okPress = okPressed();
    bool okLongPress = okLongPressed();
    bool leftPress = leftPressed();
    bool rightPress = rightPressed();

    display.clearBuffer();
    if(WiFi.status() == WL_CONNECTED)
{
    wifiConnecting = false;
    wifiConnectionFailed = false;
    wifiHasSavedCredentials = true;

    currentSSID = WiFi.SSID();
}

    drawHeader(display, batteryPercent(), currentSSID.c_str());

    if(wifiConnecting && WiFi.status() != WL_CONNECTED && millis() - wifiConnectStart > 20000)
    {
        wifiConnecting = false;
        wifiConnectionFailed = true;
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

        if(currentScreen == SCREEN_WIFI_CONNECTING)
            currentScreen = SCREEN_HOME;
    }

    // Home menu shortcut uses a single OK press.
    if(currentScreen == SCREEN_HOME && okPress)
    {
        currentScreen = SCREEN_MENU;
        okPress = false;
    }

    // Keep long-press back behavior on deeper screens.
    if(okLongPress)
    {
        switch(currentScreen)
        {
            case SCREEN_MENU:
                currentScreen = SCREEN_HOME;
                break;

            case SCREEN_SETTINGS_ANIME_LIST:
                currentScreen = SCREEN_SETTINGS_ANIME_ACTION_MENU;
                break;

            case SCREEN_WIFI_PASSWORD:
                keyboardMenu = KEY_DONE;
                currentScreen = SCREEN_KEYBOARD_MENU;
                break;

            case SCREEN_KEYBOARD_MENU:
                currentScreen = SCREEN_WIFI_PASSWORD;
                break;

            case SCREEN_SETTINGS_TITLE_MENU:
                currentScreen = SCREEN_SETTINGS;
                break;

            case SCREEN_SETTINGS_MOVIES_COMING:
                currentScreen = SCREEN_SETTINGS_TITLE_MENU;
                break;

            case SCREEN_SETTINGS_ANIME_ACTION_MENU:
                currentScreen = SCREEN_SETTINGS_ANIME_LIST;
                break;

            case SCREEN_SETTINGS:
                currentScreen = SCREEN_HOME;
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

    const int animeX = 1;
    const int movieX = 66;
    const int titleWidth = 60;
    unsigned long scrollNow = millis();


    bool showStatusOnly = (!wifiHasSavedCredentials && !wifiConnecting && !wifiConnectionFailed) || wifiConnecting || wifiConnectionFailed;

    if(showStatusOnly)
    {
        if(wifiConnectionFailed)
        {
            display.drawStr(14, 34, "Connection failed");
        }
        else if(wifiConnecting)
        {
            display.drawStr(14, 34, "Connecting to WiFi...");
        }
        else
        {
            display.drawStr(8, 30, "Connect to WiFi first");
        }
    }
    else
    {
        for(int i=0;i<3;i++)
        {
            int y = 20 + i*14;


            // Anime left
            drawScrollingText(display, animeX, y, titleWidth, animeList[i].title, scrollNow);

            display.drawStr(
                animeX,
                y+6,
                trimDisplayText(animeList[i].time, 12).c_str()
            );


            // Movie title
            drawScrollingText(display, movieX, y, titleWidth, movies[i].title, scrollNow);


            // Date
            display.drawStr(
                movieX,
                y+6,
                trimDisplayText(movies[i].date, 12).c_str()
            );
        }
    }
}


   if(currentScreen == SCREEN_MENU)
{
    // Navigation
    if(leftPress)
    {
        if(selectedMenu == MAIN_MENU_WIFI)
            selectedMenu = MAIN_MENU_ABOUT;
        else
            selectedMenu = (MainMenuItem)(selectedMenu - 1);
    }

    if(rightPress)
    {
        if(selectedMenu == MAIN_MENU_ABOUT)
            selectedMenu = MAIN_MENU_WIFI;
        else
            selectedMenu = (MainMenuItem)(selectedMenu + 1);
    }

    // Open selected item
    if(okPress)
    {
        switch(selectedMenu)
        {
            case MAIN_MENU_WIFI:
                currentScreen = SCREEN_WIFI_MENU;
                okPress = false;
                break;

            case MAIN_MENU_SETTINGS:
                currentScreen = SCREEN_SETTINGS;
                okPress = false;
                break;

            case MAIN_MENU_ABOUT:
                currentScreen = SCREEN_ABOUT;
                okPress = false;
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
    if(leftPress)
    {
        wifiMenu = (wifiMenu == WIFI_OPTION_SCAN) ? WIFI_OPTION_MANUAL : WIFI_OPTION_SCAN;
    }

    if(rightPress)
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

    if(okPress)
    {
        switch(wifiMenu)
        {
            case WIFI_OPTION_SCAN:
    wifiScanned = false;

selectedNetwork = 0;
networkTop = 0;

currentScreen = SCREEN_WIFI_SCAN;
    okPress = false;
    break;

case WIFI_OPTION_MANUAL:
    currentScreen = SCREEN_WIFI_KEYBOARD;
    okPress = false;
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
    if(leftPress)
    {
        if(selectedNetwork > 0)
        {
            selectedNetwork--;

            if(selectedNetwork < networkTop)
                networkTop--;
        }
    }

    if(rightPress)
    {
        if(selectedNetwork < networkCount - 1)
        {
            selectedNetwork++;

            if(selectedNetwork > networkTop + 3)
                networkTop++;
        }
    }

    // Select network
    if(okPress)
{
    if(networkCount > 0)
    {
        selectedSSID = networks[selectedNetwork];

        password = "";
        charIndex = 0;
        keyboardMode = MODE_LOWER;

        currentScreen = SCREEN_WIFI_PASSWORD;
        okPress = false;
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
    if(leftPress)
    {
        if(keyboardMenu == KEY_DONE)
            keyboardMenu = KEY_CANCEL;
        else
            keyboardMenu = (KeyboardMenuItem)(keyboardMenu - 1);
    }

    if(rightPress)
    {
        if(keyboardMenu == KEY_CANCEL)
            keyboardMenu = KEY_DONE;
        else
            keyboardMenu = (KeyboardMenuItem)(keyboardMenu + 1);
    }

    if(okPress)
    {
        switch(keyboardMenu)
        {
            case KEY_DONE:
            {

    wifiConnectStart = millis();
wifiConnecting = true;
wifiConnectionFailed = false;
wifiSaved = false;

bool connected = wifiConnect(selectedSSID, password);

wifiConnecting = false;
wifiConnectionFailed = !connected;
wifiHasSavedCredentials = true;

currentScreen = SCREEN_WIFI_CONNECTING;

    break;
            }

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
                password = "";
                charIndex = 0;
                currentScreen = SCREEN_HOME;
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
    else if(wifiConnectionFailed)
    {
        display.drawStr(10,25,"Connection failed");
    }
    else
    {
        display.drawStr(20,25,"Connecting...");
    }
}


if(currentScreen == SCREEN_SETTINGS)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress)
        selectedSettingsMenu = (selectedSettingsMenu == SETTINGS_DEFAULT) ? SETTINGS_SELECT_TITLE : SETTINGS_DEFAULT;

    if(rightPress)
        selectedSettingsMenu = (selectedSettingsMenu == SETTINGS_SELECT_TITLE) ? SETTINGS_DEFAULT : SETTINGS_SELECT_TITLE;

    if(okPress)
    {
        switch(selectedSettingsMenu)
        {
            case SETTINGS_DEFAULT:
                clearSelectedAnimeTitles();
                animeLoaded = false;
                currentScreen = SCREEN_HOME;
                okPress = false;
                break;

            case SETTINGS_SELECT_TITLE:
                selectedTitleSource = TITLE_SOURCE_MOVIES;
                currentScreen = SCREEN_SETTINGS_TITLE_MENU;
                okPress = false;
                break;
        }
    }

    display.drawStr(22, 26, "Settings");
    display.drawStr(10, 40, selectedSettingsMenu == SETTINGS_DEFAULT ? "> Default" : "  Default");
    display.drawStr(10, 52, selectedSettingsMenu == SETTINGS_SELECT_TITLE ? "> Select Title" : "  Select Title");
}

if(currentScreen == SCREEN_SETTINGS_TITLE_MENU)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress)
        selectedTitleSource = TITLE_SOURCE_MOVIES;

    if(rightPress)
        selectedTitleSource = TITLE_SOURCE_ANIME;

    if(okPress)
    {
        switch(selectedTitleSource)
        {
            case TITLE_SOURCE_MOVIES:
                currentScreen = SCREEN_SETTINGS_MOVIES_COMING;
                okPress = false;
                break;

            case TITLE_SOURCE_ANIME:
                fetchSeasonAnimeChoices();
                syncSelectedAnimeChoices();
                seasonAnimeCursor = 0;
                seasonAnimeTop = 0;
                currentScreen = SCREEN_SETTINGS_ANIME_LIST;
                okPress = false;
                break;
        }
    }

    display.drawStr(16, 26, "Select Title");
    display.drawStr(10, 40, selectedTitleSource == TITLE_SOURCE_MOVIES ? "> Movies" : "  Movies");
    display.drawStr(10, 52, selectedTitleSource == TITLE_SOURCE_ANIME ? "> Anime" : "  Anime");
}

if(currentScreen == SCREEN_SETTINGS_MOVIES_COMING)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(20, 26, "Movies");
    display.drawStr(12, 42, "Coming Soon");

    if(okPress)
    {
        currentScreen = SCREEN_SETTINGS_TITLE_MENU;
        okPress = false;
    }
}

if(currentScreen == SCREEN_SETTINGS_ANIME_LIST)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress && seasonAnimeCount > 0)
    {
        if(seasonAnimeCursor > 0)
            seasonAnimeCursor--;

        if(seasonAnimeCursor < seasonAnimeTop)
            seasonAnimeTop--;
    }

    if(rightPress && seasonAnimeCount > 0)
    {
        if(seasonAnimeCursor < seasonAnimeCount - 1)
        {
            seasonAnimeCursor++;

            if(seasonAnimeCursor > seasonAnimeTop + (VISIBLE_ANIME_CHOICES - 1))
                seasonAnimeTop++;
        }
    }

    if(okPress && seasonAnimeCount > 0)
    {
        seasonAnimeChoices[seasonAnimeCursor].selected = !seasonAnimeChoices[seasonAnimeCursor].selected;
        okPress = false;
    }

    if(seasonAnimeCount == 0)
    {
        display.drawStr(18, 34, "No titles");
    }
    else
    {
        for(int i = 0; i < VISIBLE_ANIME_CHOICES; i++)
        {
            int index = seasonAnimeTop + i;

            if(index >= seasonAnimeCount)
                break;

            int y = 18 + (i * 7);
            display.drawStr(2, y, index == seasonAnimeCursor ? ">" : " ");
            display.drawStr(8, y, seasonAnimeChoices[index].selected ? "*" : " ");
            drawScrollingText(display, 14, y, 110, seasonAnimeChoices[index].title, millis());
        }
    }
}

if(currentScreen == SCREEN_SETTINGS_ANIME_ACTION_MENU)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress || rightPress)
        selectedAnimeAction = (selectedAnimeAction == ANIME_ACTION_DONE) ? ANIME_ACTION_CANCEL : ANIME_ACTION_DONE;

    if(okPress)
    {
        switch(selectedAnimeAction)
        {
            case ANIME_ACTION_DONE:
                rebuildSelectedAnimeTitlesFromChoices();
                saveSelectedAnimeTitles();
                animeLoaded = false;
                selectionDoneUntil = millis() + 700;
                currentScreen = SCREEN_SETTINGS_DONE;
                okPress = false;
                break;

            case ANIME_ACTION_CANCEL:
                currentScreen = SCREEN_HOME;
                okPress = false;
                break;
        }
    }

    display.drawStr(22, 26, "Finish?");
    display.drawStr(14, 40, selectedAnimeAction == ANIME_ACTION_DONE ? "> Done" : "  Done");
    display.drawStr(14, 52, selectedAnimeAction == ANIME_ACTION_CANCEL ? "> Cancel" : "  Cancel");
}

if(currentScreen == SCREEN_SETTINGS_DONE)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(24, 40, "Done");

    if(millis() > selectionDoneUntil)
        currentScreen = SCREEN_HOME;
}

if(currentScreen == SCREEN_ABOUT)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(35,35,"About");
}


    display.sendBuffer();

    delay(10);
}