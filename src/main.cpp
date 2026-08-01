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
#include "pins.h"
#include <esp_sleep.h>
#include <driver/gpio.h>


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
bool movieFetchFailed = false;
bool animeFetchFailed = false;
long selectedAnimeAlertAiringAt = 0;
long selectedAnimeAlertMutedAiringAt = 0;
bool selectedAnimeAlertBeepOn = false;
unsigned long selectedAnimeAlertToggleAt = 0;
unsigned long leftLedFlashStartedAt = 0;
unsigned long leftLedFlashUntil = 0;

const int RIGHT_LED_PWM_CHANNEL = 0;
const int RIGHT_LED_PWM_FREQUENCY = 5000;
const int RIGHT_LED_PWM_RESOLUTION = 8;

const unsigned char WIFI_ICON_BITMAP[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x3F, 0xFC, 0x00,
    0x01, 0xFF, 0xFF, 0x00,
    0x07, 0xF8, 0x3F, 0xE0,
    0x1F, 0x80, 0x03, 0xF0,
    0x3E, 0x00, 0x00, 0x7C,
    0x78, 0x00, 0x00, 0x3E,
    0xF0, 0x0F, 0xF0, 0x0F,
    0xE0, 0x7F, 0xFC, 0x07,
    0x00, 0xFF, 0xFF, 0x00,
    0x03, 0xF0, 0x0F, 0x80,
    0x07, 0xC0, 0x03, 0xE0,
    0x0F, 0x00, 0x00, 0xE0,
    0x06, 0x03, 0xC0, 0x60,
    0x00, 0x1F, 0xF8, 0x00,
    0x00, 0x3F, 0xFC, 0x00,
    0x00, 0x78, 0x1E, 0x00,
    0x00, 0x60, 0x06, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x03, 0xC0, 0x00,
    0x00, 0x07, 0xE0, 0x00,
    0x00, 0x07, 0xE0, 0x00,
    0x00, 0x07, 0xE0, 0x00,
    0x00, 0x03, 0xC0, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char SETTINGS_ICON_BITMAP[] PROGMEM = {
    0x00, 0x03, 0xE0, 0x00,
    0x00, 0x07, 0xF0, 0x00,
    0x00, 0x07, 0xF0, 0x00,
    0x03, 0x87, 0xF0, 0x40,
    0x07, 0xDF, 0xF8, 0xE0,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF8,
    0x0F, 0xFF, 0xFF, 0xF8,
    0x07, 0xFF, 0xFF, 0xF8,
    0x07, 0xFF, 0xFF, 0xF0,
    0x07, 0xF8, 0x1F, 0xE0,
    0x0F, 0xE0, 0x07, 0xF0,
    0x7F, 0xE0, 0x07, 0xF0,
    0xFF, 0xC0, 0x03, 0xFE,
    0xFF, 0xC0, 0x03, 0xFF,
    0xFF, 0xC0, 0x03, 0xFF,
    0xFF, 0xC0, 0x03, 0xFF,
    0xFF, 0xC0, 0x03, 0xFF,
    0x7F, 0xC0, 0x03, 0xFF,
    0x0F, 0xE0, 0x07, 0xFE,
    0x0F, 0xE0, 0x07, 0xF0,
    0x07, 0xF8, 0x1F, 0xE0,
    0x0F, 0xFF, 0xFF, 0xE0,
    0x1F, 0xFF, 0xFF, 0xE0,
    0x1F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF8,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x07, 0x1F, 0xFB, 0xE0,
    0x02, 0x0F, 0xE1, 0xC0,
    0x00, 0x0F, 0xE0, 0x00,
    0x00, 0x0F, 0xE0, 0x00,
    0x00, 0x07, 0xC0, 0x00,
};

const unsigned char ABOUT_ICON_BITMAP[] PROGMEM = {
    0x00, 0x0F, 0xF0, 0x00,
    0x00, 0x7F, 0xFE, 0x00,
    0x01, 0xFF, 0xFF, 0x80,
    0x03, 0xFF, 0xFF, 0xC0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFC, 0x1F, 0xF8,
    0x3F, 0xF8, 0x1F, 0xFC,
    0x3F, 0xF8, 0x1F, 0xFC,
    0x7F, 0xF8, 0x3F, 0xFE,
    0x7F, 0xFC, 0x7F, 0xFE,
    0x7F, 0xFF, 0xFF, 0xFE,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xE0, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0xFF, 0xF8, 0x1F, 0xFF,
    0x7F, 0xF8, 0x1F, 0xFE,
    0x7F, 0xF8, 0x1F, 0xFE,
    0x7F, 0xF8, 0x1F, 0xFE,
    0x3F, 0xF8, 0x1F, 0xFC,
    0x3F, 0xE0, 0x07, 0xFC,
    0x1F, 0xF0, 0x0F, 0xF8,
    0x0F, 0xFF, 0xFF, 0xF0,
    0x07, 0xFF, 0xFF, 0xE0,
    0x03, 0xFF, 0xFF, 0xC0,
    0x01, 0xFF, 0xFF, 0x80,
    0x00, 0x7F, 0xFE, 0x00,
    0x00, 0x0F, 0xF0, 0x00,
};

const unsigned char LIST_ICON_BITMAP[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x30, 0x00, 0x00, 0x00,
    0x79, 0xFF, 0xFF, 0xFC,
    0x7B, 0xFF, 0xFF, 0xFE,
    0x79, 0xFF, 0xFF, 0xFC,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x78, 0x00, 0x00, 0x00,
    0x7B, 0xFF, 0xFF, 0xFE,
    0x79, 0xFF, 0xFF, 0xFC,
    0x38, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x00, 0x00,
    0x79, 0xFF, 0xFF, 0xFC,
    0x7B, 0xFF, 0xFF, 0xFE,
    0x78, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x79, 0xFF, 0xFF, 0xFC,
    0x7B, 0xFF, 0xFF, 0xFE,
    0x79, 0xFF, 0xFF, 0xFC,
    0x30, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char REBOOT_ICON_BITMAP[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0xE1, 0x00,
    0x00, 0x3F, 0xFF, 0x80,
    0x00, 0xFF, 0xFF, 0x80,
    0x01, 0xF0, 0x1F, 0xC0,
    0x03, 0xC0, 0x07, 0xC0,
    0x07, 0x80, 0x0F, 0xE0,
    0x07, 0x00, 0x00, 0x00,
    0x0E, 0x00, 0x00, 0x00,
    0x0E, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x00, 0x18,
    0x1C, 0x00, 0x00, 0x18,
    0x1C, 0x00, 0x00, 0x30,
    0x0C, 0x00, 0x00, 0x30,
    0x0E, 0x00, 0x00, 0x30,
    0x0E, 0x00, 0x00, 0x60,
    0x07, 0x00, 0x00, 0xE0,
    0x07, 0x80, 0x00, 0xC0,
    0x03, 0xC0, 0x03, 0xC0,
    0x01, 0xE0, 0x07, 0x80,
    0x00, 0xFC, 0x3E, 0x00,
    0x00, 0x3F, 0xFC, 0x00,
    0x00, 0x07, 0xE0, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char BATTERY_ICON_BITMAP[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x1F, 0xFF, 0xFF, 0xF0,
    0x3F, 0xFF, 0xFF, 0xF0,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xFE,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xF8,
    0x3F, 0xFF, 0xFF, 0xF0,
    0x1F, 0xFF, 0xFF, 0xF0,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

const unsigned char ABOUT_SCREEN_BITMAP[] PROGMEM = {
    0xE0, 0xFF, 0xFF, 0x07, 0x00,
    0xF0, 0xFF, 0xFF, 0x0F, 0x00,
    0xF8, 0xFF, 0xFF, 0x0F, 0x00,
    0xF8, 0xFF, 0xFF, 0x0F, 0x00,
    0xFC, 0xFF, 0xFF, 0x1F, 0x00,
    0xFE, 0xFF, 0xFF, 0x7F, 0x00,
    0xFE, 0xFF, 0xFF, 0xFF, 0x00,
    0xFE, 0xFF, 0xFF, 0xFF, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0x01,
    0xFF, 0xFF, 0xFF, 0xFE, 0x01,
    0xFF, 0xFF, 0x3F, 0xFC, 0x00,
    0xFF, 0xFF, 0x1F, 0x30, 0x00,
    0xFF, 0xFF, 0x0F, 0x70, 0x00,
    0xFE, 0xFF, 0x3F, 0xF8, 0x0F,
    0xFE, 0xFF, 0xFF, 0xFF, 0x07,
    0xFE, 0xE3, 0xFF, 0xFF, 0x05,
    0xE6, 0xE3, 0xFF, 0xFF, 0x05,
    0xC6, 0xC3, 0xFF, 0xFF, 0x05,
    0x00, 0xC3, 0xFF, 0xFE, 0x04,
    0x01, 0xC3, 0xFF, 0xFC, 0x04,
    0x03, 0x82, 0x7F, 0xFC, 0x01,
    0x67, 0x02, 0x7F, 0x78, 0x00,
    0xC7, 0x02, 0x78, 0x00, 0x00,
    0x0F, 0x02, 0x00, 0x00, 0x00,
    0x1F, 0x02, 0x80, 0x38, 0x00,
    0x1F, 0x06, 0xF8, 0xFF, 0x00,
    0x7F, 0x06, 0xFC, 0xFD, 0x00,
    0xFF, 0x0E, 0xFE, 0xFF, 0x00,
    0x7F, 0x1C, 0x7C, 0x40, 0x00,
    0x3F, 0x1C, 0x00, 0x00, 0x00,
    0x1F, 0x3C, 0x00, 0x02, 0x00,
    0x1F, 0x38, 0x00, 0x2E, 0x01,
    0x1F, 0x38, 0x00, 0x3F, 0x00,
    0x0F, 0x78, 0xF8, 0x3F, 0x00,
    0x00, 0xF0, 0xFF, 0x3F, 0x00,
    0x00, 0xC0, 0xFF, 0x3F, 0x00,
    0x00, 0x00, 0xFE, 0x3F, 0x00,
    0x00, 0x00, 0xE0, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
};

const char *ABOUT_INFO_LINES[] = {
    "Created by Rafiul Rafi",
    "rafiulrafi55@gmail.com",
    "github/rafiulrafi55"
};

const int ABOUT_INFO_LINE_COUNT = 3;

const unsigned long LOAD_RETRY_INTERVAL = 15000;
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
    SCREEN_SETTINGS_EMPTY,
    SCREEN_SETTINGS_TITLE_MENU,
    SCREEN_SETTINGS_MOVIES_COMING,
    SCREEN_SETTINGS_ANIME_LIST,
    SCREEN_SETTINGS_ANIME_ACTION_MENU,
    SCREEN_SETTINGS_DONE,
    SCREEN_REBOOT_CONFIRM,
    SCREEN_BATTERY_INFO,
    SCREEN_ABOUT,
    SCREEN_TITLE_DETAILS
};

enum MainMenuItem
{
    MAIN_MENU_WIFI,
    MAIN_MENU_SETTINGS,
    MAIN_MENU_LIST,
    MAIN_MENU_BATTERY,
    MAIN_MENU_ABOUT,
    MAIN_MENU_REBOOT
};

enum RebootActionItem
{
    REBOOT_ACTION_REBOOT,
    REBOOT_ACTION_POWER_OFF
};

enum SettingsMenuItem
{
    SETTINGS_DEFAULT,
    SETTINGS_SELECT_TITLE
};

enum DeviceSettingsItem
{
    DEVICE_SETTING_BRIGHTNESS,
    DEVICE_SETTING_LED,
    DEVICE_SETTING_BUZZER,
    DEVICE_SETTING_LOW_POWER,
    DEVICE_SETTING_AUTO_LOW_POWER_TRIGGER,
    DEVICE_SETTING_BUZZER_LEAD_TIME,
    DEVICE_SETTING_REFRESH_INTERVAL,
    DEVICE_SETTING_HOME_CURSOR_DEFAULT,
    DEVICE_SETTING_DETAIL_AUTORETURN,
    DEVICE_SETTING_SCREEN_TIMEOUT,
    DEVICE_SETTING_CLOCK_FORMAT,
    DEVICE_SETTING_DESCRIPTION_SOURCE
};

const int DEVICE_SETTINGS_COUNT = 12;

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

enum SelectionMode
{
    SELECTION_MODE_ANIME,
    SELECTION_MODE_MOVIES
};

enum HomeDetailSource
{
    HOME_DETAIL_ANIME,
    HOME_DETAIL_MOVIE
};

Screen currentScreen = SCREEN_HOME;
MainMenuItem selectedMenu = MAIN_MENU_WIFI;
SettingsMenuItem selectedSettingsMenu = SETTINGS_DEFAULT;
TitleSourceItem selectedTitleSource = TITLE_SOURCE_MOVIES;
AnimeActionItem selectedAnimeAction = ANIME_ACTION_DONE;
SelectionMode selectionMode = SELECTION_MODE_ANIME;
RebootActionItem rebootActionItem = REBOOT_ACTION_REBOOT;
DeviceSettingsItem selectedDeviceSetting = DEVICE_SETTING_BRIGHTNESS;
int aboutInfoLineIndex = 0;
bool homeCursorActive = false;
int homeCursorIndex = 0;
HomeDetailSource homeDetailSource = HOME_DETAIL_ANIME;
int homeDetailItemIndex = 0;
int homeDetailScrollRow = 0;
unsigned long homeDetailOpenedAt = 0;
int settingsMenuTop = 0;
int refreshIntervalIndex = 1;
bool homeCursorDefaultEnabled = false;
int detailAutoReturnIndex = 1;
int screenTimeoutIndex = 2;
bool clock24HourEnabled = true;
bool detailSummaryOnly = false;
int autoLowPowerTriggerIndex = 1;
int buzzerLeadTimeIndex = 3;
const int HOME_CURSOR_SLOT_COUNT = 6;
const int DETAIL_CHARS_PER_ROW = 31;
const int DETAIL_VISIBLE_ROWS = 8;
const int SETTINGS_VISIBLE_ITEMS = 5;

const unsigned long REFRESH_INTERVAL_OPTIONS[] = {300000UL, 900000UL, 1800000UL, 3600000UL};
const char *REFRESH_INTERVAL_LABELS[] = {"5m", "15m", "30m", "60m"};
const int REFRESH_INTERVAL_COUNT = sizeof(REFRESH_INTERVAL_OPTIONS) / sizeof(REFRESH_INTERVAL_OPTIONS[0]);

const int AUTO_LOW_POWER_TRIGGER_OPTIONS[] = {5, 10, 15, -1};
const char *AUTO_LOW_POWER_TRIGGER_LABELS[] = {"5%", "10%", "15%", "Never"};
const int AUTO_LOW_POWER_TRIGGER_COUNT = sizeof(AUTO_LOW_POWER_TRIGGER_OPTIONS) / sizeof(AUTO_LOW_POWER_TRIGGER_OPTIONS[0]);

const unsigned long BUZZER_LEAD_TIME_OPTIONS[] = {120000UL, 300000UL, 600000UL, 0UL};
const char *BUZZER_LEAD_TIME_LABELS[] = {"2m", "5m", "10m", "On time"};
const int BUZZER_LEAD_TIME_COUNT = sizeof(BUZZER_LEAD_TIME_OPTIONS) / sizeof(BUZZER_LEAD_TIME_OPTIONS[0]);

const unsigned long DETAIL_AUTORETURN_OPTIONS[] = {0UL, 10000UL, 30000UL, 60000UL};
const char *DETAIL_AUTORETURN_LABELS[] = {"Off", "10s", "30s", "60s"};
const int DETAIL_AUTORETURN_COUNT = sizeof(DETAIL_AUTORETURN_OPTIONS) / sizeof(DETAIL_AUTORETURN_OPTIONS[0]);

const unsigned long SCREEN_TIMEOUT_OPTIONS[] = {0UL, 30000UL, 60000UL, 120000UL, 300000UL};
const char *SCREEN_TIMEOUT_LABELS[] = {"Off", "30s", "1m", "2m", "5m"};
const int SCREEN_TIMEOUT_COUNT = sizeof(SCREEN_TIMEOUT_OPTIONS) / sizeof(SCREEN_TIMEOUT_OPTIONS[0]);

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

int brightnessLevelIndex = 2;
bool ledAlertsEnabled = true;
bool buzzerAlertsEnabled = true;
bool lowPowerModeEnabled = false;
bool displaySleeping = false;
unsigned long lastInteractionAt = 0;
unsigned long wakeIgnoreUntil = 0;
bool lowPowerManualOverrideOff = false;
const uint8_t BRIGHTNESS_LEVELS[] = {64, 128, 192, 255};
const int BRIGHTNESS_LEVEL_COUNT = sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);
const unsigned long LOW_POWER_WAKE_IGNORE_MS = 600;

void setRightLedBrightness(uint8_t duty);
void enterLowPowerSleep();
void enterMenuPowerOffSleep();
void refreshAutomaticLowPowerMode(int batteryLevel);
void triggerLeftLedFlashPattern(unsigned long durationMs);
bool displayedMoviesChanged(const String previousTitles[3], const String previousDates[3]);
bool displayedAnimeChanged(const String previousTitles[3], const String previousTimes[3], const long previousAiringAt[3]);

AnimeChoice seasonAnimeChoices[MAX_ANIME_CHOICES];
int seasonAnimeCount = 0;
String selectedAnimeTitles[MAX_ANIME_CHOICES];
int selectedAnimeTitleCount = 0;
int seasonAnimeCursor = 0;
int seasonAnimeTop = 0;
int seasonMovieCursor = 0;
int seasonMovieTop = 0;
unsigned long selectionDoneUntil = 0;
const int VISIBLE_ANIME_CHOICES = 7;
const int VISIBLE_MOVIE_CHOICES = 7;

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

bool movieTitleIsSelected(const String &title)
{
    for(int i = 0; i < selectedMovieTitleCount; i++)
    {
        if(selectedMovieTitles[i] == title)
            return true;
    }

    return false;
}

void loadSelectedMovieTitles()
{
    selectedMovieTitleCount = 0;

    prefs.begin("movie", true);
    String stored = prefs.getString("titles", "");
    prefs.end();

    while(stored.length() > 0 && selectedMovieTitleCount < MAX_MOVIE_CHOICES)
    {
        int separator = stored.indexOf('\n');
        String entry = separator >= 0 ? stored.substring(0, separator) : stored;
        entry.trim();

        if(entry.length() > 0)
            selectedMovieTitles[selectedMovieTitleCount++] = entry;

        if(separator < 0)
            break;

        stored = stored.substring(separator + 1);
    }
}

void saveSelectedMovieTitles()
{
    String stored;

    for(int i = 0; i < selectedMovieTitleCount; i++)
    {
        if(i > 0)
            stored += '\n';

        stored += selectedMovieTitles[i];
    }

    prefs.begin("movie", false);
    prefs.putString("titles", stored);
    prefs.end();
}

void clearSelectedAnimeTitles()
{
    selectedAnimeTitleCount = 0;
    selectedAnimeAlertAiringAt = 0;
    selectedAnimeAlertMutedAiringAt = 0;
    selectedAnimeAlertBeepOn = false;
    digitalWrite(BUZZER_PIN, LOW);
    prefs.begin("anime", false);
    prefs.putString("titles", "");
    prefs.end();
}

void clearSelectedMovieTitles()
{
    selectedMovieTitleCount = 0;
    prefs.begin("movie", false);
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

void syncSelectedMovieChoices()
{
    for(int i = 0; i < seasonMovieCount; i++)
        seasonMovieChoices[i].selected = movieTitleIsSelected(seasonMovieChoices[i].title);
}

void rebuildSelectedMovieTitlesFromChoices()
{
    selectedMovieTitleCount = 0;

    for(int i = 0; i < seasonMovieCount; i++)
    {
        if(!seasonMovieChoices[i].selected)
            continue;

        if(selectedMovieTitleCount >= MAX_MOVIE_CHOICES)
            break;

        selectedMovieTitles[selectedMovieTitleCount++] = seasonMovieChoices[i].title;
    }
}

void saveUiSettings()
{
    prefs.begin("settings", false);
    prefs.putInt("brightness", brightnessLevelIndex);
    prefs.putBool("led", ledAlertsEnabled);
    prefs.putBool("buzzer", buzzerAlertsEnabled);
    prefs.putBool("low_power", lowPowerModeEnabled);
    prefs.putInt("auto_lp_idx", autoLowPowerTriggerIndex);
    prefs.putInt("refresh_idx", refreshIntervalIndex);
    prefs.putBool("cursor_default", homeCursorDefaultEnabled);
    prefs.putInt("detail_auto", detailAutoReturnIndex);
    prefs.putInt("screen_timeout", screenTimeoutIndex);
    prefs.putBool("clock_24", clock24HourEnabled);
    prefs.putBool("detail_summary", detailSummaryOnly);
    prefs.putInt("buzzer_lead_idx", buzzerLeadTimeIndex);
    prefs.end();
}

int effectiveBrightnessLevelIndex()
{
    return lowPowerModeEnabled ? 0 : brightnessLevelIndex;
}

bool ledAlertsActive()
{
    return ledAlertsEnabled && !lowPowerModeEnabled;
}

bool buzzerAlertsActive()
{
    return buzzerAlertsEnabled && !lowPowerModeEnabled;
}

const char* brightnessLabelForIndex(int levelIndex)
{
    if(levelIndex <= 0)
        return "Low";
    if(levelIndex == 1)
        return "Med";
    if(levelIndex == 2)
        return "High";
    return "Max";
}

void applyDisplayBrightness()
{
    if(brightnessLevelIndex < 0)
        brightnessLevelIndex = 0;

    if(brightnessLevelIndex >= BRIGHTNESS_LEVEL_COUNT)
        brightnessLevelIndex = BRIGHTNESS_LEVEL_COUNT - 1;

    display.setContrast(BRIGHTNESS_LEVELS[effectiveBrightnessLevelIndex()]);
}

void applyLowPowerModeState()
{
    applyDisplayBrightness();

    if(lowPowerModeEnabled)
    {
        selectedAnimeAlertBeepOn = false;
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LEFT_LED_PIN, LOW);
        setRightLedBrightness(0);
        lastInteractionAt = millis();
    }
    else
    {
        displaySleeping = false;
        display.setPowerSave(0);
    }
}

void enterLowPowerSleep()
{
    display.setPowerSave(1);
    gpio_wakeup_enable(static_cast<gpio_num_t>(BTN_OK), GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);

    displaySleeping = false;
    display.setPowerSave(0);
    applyDisplayBrightness();
    lastInteractionAt = millis();
    wakeIgnoreUntil = millis() + LOW_POWER_WAKE_IGNORE_MS;
}

void enterMenuPowerOffSleep()
{
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LEFT_LED_PIN, LOW);
    setRightLedBrightness(0);

    display.setPowerSave(1);
    gpio_wakeup_enable(static_cast<gpio_num_t>(BTN_OK), GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);

    display.setPowerSave(0);
    currentScreen = SCREEN_HOME;
    homeCursorActive = false;
    displaySleeping = false;
    applyDisplayBrightness();
    lastInteractionAt = millis();
    wakeIgnoreUntil = millis() + LOW_POWER_WAKE_IGNORE_MS;
}

void loadUiSettings()
{
    prefs.begin("settings", true);
    brightnessLevelIndex = prefs.getInt("brightness", brightnessLevelIndex);
    ledAlertsEnabled = prefs.getBool("led", true);
    buzzerAlertsEnabled = prefs.getBool("buzzer", true);
    lowPowerModeEnabled = prefs.getBool("low_power", false);
    autoLowPowerTriggerIndex = prefs.getInt("auto_lp_idx", autoLowPowerTriggerIndex);
    refreshIntervalIndex = prefs.getInt("refresh_idx", refreshIntervalIndex);
    homeCursorDefaultEnabled = prefs.getBool("cursor_default", false);
    detailAutoReturnIndex = prefs.getInt("detail_auto", detailAutoReturnIndex);
    screenTimeoutIndex = prefs.getInt("screen_timeout", screenTimeoutIndex);
    clock24HourEnabled = prefs.getBool("clock_24", true);
    detailSummaryOnly = prefs.getBool("detail_summary", false);
    buzzerLeadTimeIndex = prefs.getInt("buzzer_lead_idx", buzzerLeadTimeIndex);
    prefs.end();

    if(brightnessLevelIndex < 0)
        brightnessLevelIndex = 0;

    if(brightnessLevelIndex >= BRIGHTNESS_LEVEL_COUNT)
        brightnessLevelIndex = BRIGHTNESS_LEVEL_COUNT - 1;

    if(refreshIntervalIndex < 0)
        refreshIntervalIndex = 0;
    if(refreshIntervalIndex >= REFRESH_INTERVAL_COUNT)
        refreshIntervalIndex = REFRESH_INTERVAL_COUNT - 1;

    if(autoLowPowerTriggerIndex < 0)
        autoLowPowerTriggerIndex = 0;
    if(autoLowPowerTriggerIndex >= AUTO_LOW_POWER_TRIGGER_COUNT)
        autoLowPowerTriggerIndex = AUTO_LOW_POWER_TRIGGER_COUNT - 1;

    if(buzzerLeadTimeIndex < 0)
        buzzerLeadTimeIndex = 0;
    if(buzzerLeadTimeIndex >= BUZZER_LEAD_TIME_COUNT)
        buzzerLeadTimeIndex = BUZZER_LEAD_TIME_COUNT - 1;

    if(detailAutoReturnIndex < 0)
        detailAutoReturnIndex = 0;
    if(detailAutoReturnIndex >= DETAIL_AUTORETURN_COUNT)
        detailAutoReturnIndex = DETAIL_AUTORETURN_COUNT - 1;

    if(screenTimeoutIndex < 0)
        screenTimeoutIndex = 0;
    if(screenTimeoutIndex >= SCREEN_TIMEOUT_COUNT)
        screenTimeoutIndex = SCREEN_TIMEOUT_COUNT - 1;

    homeCursorActive = homeCursorDefaultEnabled;
}

String currentTimeLabel()
{
    if(!timeReady())
        return "--:--";

    time_t nowRaw = time(nullptr);
    struct tm nowTime;
    localtime_r(&nowRaw, &nowTime);

    char buffer[12];
    if(clock24HourEnabled)
    {
        snprintf(buffer, sizeof(buffer), "%02d:%02d", nowTime.tm_hour, nowTime.tm_min);
    }
    else
    {
        int hour = nowTime.tm_hour;
        const char *suffix = hour >= 12 ? "PM" : "AM";
        hour = hour % 12;
        if(hour == 0)
            hour = 12;

        snprintf(buffer, sizeof(buffer), "%d:%02d%s", hour, nowTime.tm_min, suffix);
    }

    return String(buffer);
}

unsigned long refreshIntervalMs()
{
    if(refreshIntervalIndex < 0)
        refreshIntervalIndex = 0;

    if(refreshIntervalIndex >= REFRESH_INTERVAL_COUNT)
        refreshIntervalIndex = REFRESH_INTERVAL_COUNT - 1;

    return REFRESH_INTERVAL_OPTIONS[refreshIntervalIndex];
}

int autoLowPowerTriggerPercent()
{
    if(autoLowPowerTriggerIndex < 0)
        autoLowPowerTriggerIndex = 0;

    if(autoLowPowerTriggerIndex >= AUTO_LOW_POWER_TRIGGER_COUNT)
        autoLowPowerTriggerIndex = AUTO_LOW_POWER_TRIGGER_COUNT - 1;

    return AUTO_LOW_POWER_TRIGGER_OPTIONS[autoLowPowerTriggerIndex];
}

unsigned long buzzerLeadTimeMs()
{
    if(buzzerLeadTimeIndex < 0)
        buzzerLeadTimeIndex = 0;

    if(buzzerLeadTimeIndex >= BUZZER_LEAD_TIME_COUNT)
        buzzerLeadTimeIndex = BUZZER_LEAD_TIME_COUNT - 1;

    return BUZZER_LEAD_TIME_OPTIONS[buzzerLeadTimeIndex];
}

unsigned long detailAutoReturnMs()
{
    if(detailAutoReturnIndex < 0)
        detailAutoReturnIndex = 0;

    if(detailAutoReturnIndex >= DETAIL_AUTORETURN_COUNT)
        detailAutoReturnIndex = DETAIL_AUTORETURN_COUNT - 1;

    return DETAIL_AUTORETURN_OPTIONS[detailAutoReturnIndex];
}

unsigned long screenTimeoutMs()
{
    if(screenTimeoutIndex < 0)
        screenTimeoutIndex = 0;

    if(screenTimeoutIndex >= SCREEN_TIMEOUT_COUNT)
        screenTimeoutIndex = SCREEN_TIMEOUT_COUNT - 1;

    return SCREEN_TIMEOUT_OPTIONS[screenTimeoutIndex];
}

String shortenDescription(const String &text)
{
    if(text.length() == 0)
        return "No description available.";

    int sentenceEnd = text.indexOf('.');
    if(sentenceEnd > 0)
    {
        int endIndex = sentenceEnd + 1;
        if(endIndex > 160)
            endIndex = 160;

        return text.substring(0, endIndex);
    }

    if(text.length() > 160)
        return text.substring(0, 160);

    return text;
}

String activeDetailDescription();

String detailDescriptionForScreen()
{
    String description = activeDetailDescription();
    return detailSummaryOnly ? shortenDescription(description) : description;
}

const char *selectedRefreshIntervalLabel()
{
    return REFRESH_INTERVAL_LABELS[refreshIntervalIndex];
}

const char *selectedDetailAutoReturnLabel()
{
    return DETAIL_AUTORETURN_LABELS[detailAutoReturnIndex];
}

const char *selectedScreenTimeoutLabel()
{
    return SCREEN_TIMEOUT_LABELS[screenTimeoutIndex];
}

String selectedClockFormatLabel()
{
    return clock24HourEnabled ? String("24 Hour") : String("12 Hour");
}

String selectedDescriptionSourceLabel()
{
    return detailSummaryOnly ? String("Short") : String("Full");
}

String selectedAutoLowPowerTriggerLabel()
{
    return String(AUTO_LOW_POWER_TRIGGER_LABELS[autoLowPowerTriggerIndex]);
}

String selectedBuzzerLeadTimeLabel()
{
    return String(BUZZER_LEAD_TIME_LABELS[buzzerLeadTimeIndex]);
}

const char *deviceSettingLabel(DeviceSettingsItem item)
{
    switch(item)
    {
        case DEVICE_SETTING_BRIGHTNESS:
            return "Brightness";
        case DEVICE_SETTING_LED:
            return "LED Alerts";
        case DEVICE_SETTING_BUZZER:
            return "Buzzer";
        case DEVICE_SETTING_LOW_POWER:
            return "Low Power";
        case DEVICE_SETTING_AUTO_LOW_POWER_TRIGGER:
            return "Auto Low Power";
        case DEVICE_SETTING_BUZZER_LEAD_TIME:
            return "Buzzer Alert";
        case DEVICE_SETTING_REFRESH_INTERVAL:
            return "Refresh Interval";
        case DEVICE_SETTING_HOME_CURSOR_DEFAULT:
            return "Home Cursor";
        case DEVICE_SETTING_DETAIL_AUTORETURN:
            return "Detail Return";
        case DEVICE_SETTING_SCREEN_TIMEOUT:
            return "Screen Sleep";
        case DEVICE_SETTING_CLOCK_FORMAT:
            return "Clock Format";
        case DEVICE_SETTING_DESCRIPTION_SOURCE:
            return "Desc Source";
    }

    return "Setting";
}

String deviceSettingValue(DeviceSettingsItem item)
{
    switch(item)
    {
        case DEVICE_SETTING_BRIGHTNESS:
            return String(brightnessLabelForIndex(effectiveBrightnessLevelIndex()));

        case DEVICE_SETTING_LED:
            return ledAlertsEnabled ? String("On") : String("Off");

        case DEVICE_SETTING_BUZZER:
            return buzzerAlertsEnabled ? String("On") : String("Off");

        case DEVICE_SETTING_LOW_POWER:
            return lowPowerModeEnabled ? String("On") : String("Off");

        case DEVICE_SETTING_AUTO_LOW_POWER_TRIGGER:
            return selectedAutoLowPowerTriggerLabel();

        case DEVICE_SETTING_BUZZER_LEAD_TIME:
            return selectedBuzzerLeadTimeLabel();

        case DEVICE_SETTING_REFRESH_INTERVAL:
            return String(selectedRefreshIntervalLabel());

        case DEVICE_SETTING_HOME_CURSOR_DEFAULT:
            return homeCursorDefaultEnabled ? String("On") : String("Off");

        case DEVICE_SETTING_DETAIL_AUTORETURN:
            return String(selectedDetailAutoReturnLabel());

        case DEVICE_SETTING_SCREEN_TIMEOUT:
            return String(selectedScreenTimeoutLabel());

        case DEVICE_SETTING_CLOCK_FORMAT:
            return selectedClockFormatLabel();

        case DEVICE_SETTING_DESCRIPTION_SOURCE:
            return selectedDescriptionSourceLabel();
    }

    return String("Off");
}

void changeDeviceSettingValue(DeviceSettingsItem item)
{
    switch(item)
    {
        case DEVICE_SETTING_BRIGHTNESS:
            brightnessLevelIndex++;
            if(brightnessLevelIndex >= BRIGHTNESS_LEVEL_COUNT)
                brightnessLevelIndex = 0;

            applyDisplayBrightness();
            break;

        case DEVICE_SETTING_LED:
            ledAlertsEnabled = !ledAlertsEnabled;
            if(!ledAlertsEnabled)
            {
                digitalWrite(LEFT_LED_PIN, LOW);
                setRightLedBrightness(0);
            }
            break;

        case DEVICE_SETTING_BUZZER:
            buzzerAlertsEnabled = !buzzerAlertsEnabled;
            if(!buzzerAlertsEnabled)
            {
                selectedAnimeAlertBeepOn = false;
                digitalWrite(BUZZER_PIN, LOW);
            }
            else if(!lowPowerModeEnabled)
            {
                digitalWrite(BUZZER_PIN, HIGH);
                delay(70);
                digitalWrite(BUZZER_PIN, LOW);
            }
            break;

        case DEVICE_SETTING_LOW_POWER:
            lowPowerModeEnabled = !lowPowerModeEnabled;
            if(lowPowerModeEnabled)
                lowPowerManualOverrideOff = false;
            else
                lowPowerManualOverrideOff = true;
            applyLowPowerModeState();
            break;

        case DEVICE_SETTING_AUTO_LOW_POWER_TRIGGER:
            autoLowPowerTriggerIndex = (autoLowPowerTriggerIndex + 1) % AUTO_LOW_POWER_TRIGGER_COUNT;
            break;

        case DEVICE_SETTING_BUZZER_LEAD_TIME:
            buzzerLeadTimeIndex = (buzzerLeadTimeIndex + 1) % BUZZER_LEAD_TIME_COUNT;
            break;

        case DEVICE_SETTING_REFRESH_INTERVAL:
            refreshIntervalIndex = (refreshIntervalIndex + 1) % REFRESH_INTERVAL_COUNT;
            break;

        case DEVICE_SETTING_HOME_CURSOR_DEFAULT:
            homeCursorDefaultEnabled = !homeCursorDefaultEnabled;
            homeCursorActive = homeCursorDefaultEnabled;
            break;

        case DEVICE_SETTING_DETAIL_AUTORETURN:
            detailAutoReturnIndex = (detailAutoReturnIndex + 1) % DETAIL_AUTORETURN_COUNT;
            break;

        case DEVICE_SETTING_SCREEN_TIMEOUT:
            screenTimeoutIndex = (screenTimeoutIndex + 1) % SCREEN_TIMEOUT_COUNT;
            break;

        case DEVICE_SETTING_CLOCK_FORMAT:
            clock24HourEnabled = !clock24HourEnabled;
            break;

        case DEVICE_SETTING_DESCRIPTION_SOURCE:
            detailSummaryOnly = !detailSummaryOnly;
            break;
    }

    saveUiSettings();
}

String trimDisplayText(const String &text, size_t maxChars)
{
    if (text.length() <= maxChars)
        return text;

    return text.substring(0, maxChars);
}

bool isSameLocalDay(time_t first, time_t second)
{
    struct tm firstTime;
    struct tm secondTime;

    localtime_r(&first, &firstTime);
    localtime_r(&second, &secondTime);

    return firstTime.tm_year == secondTime.tm_year && firstTime.tm_yday == secondTime.tm_yday;
}

void silenceSelectedAnimeAlert()
{
    if(selectedAnimeAlertAiringAt > 0)
        selectedAnimeAlertMutedAiringAt = selectedAnimeAlertAiringAt;

    selectedAnimeAlertBeepOn = false;
    digitalWrite(BUZZER_PIN, LOW);
}

bool homeCursorSelectionAvailable()
{
    if(homeCursorIndex < 0 || homeCursorIndex >= HOME_CURSOR_SLOT_COUNT)
        return false;

    if(homeCursorIndex < 3)
        return animeList[homeCursorIndex].title.length() > 0;

    int movieIndex = homeCursorIndex - 3;
    return movies[movieIndex].title.length() > 0;
}

void openHomeCursorDetails()
{
    if(!homeCursorSelectionAvailable())
        return;

    if(homeCursorIndex < 3)
    {
        homeDetailSource = HOME_DETAIL_ANIME;
        homeDetailItemIndex = homeCursorIndex;
    }
    else
    {
        homeDetailSource = HOME_DETAIL_MOVIE;
        homeDetailItemIndex = homeCursorIndex - 3;
    }

    homeDetailScrollRow = 0;
    homeDetailOpenedAt = millis();
    currentScreen = SCREEN_TITLE_DETAILS;
}

String activeDetailDescription()
{
    String description;

    if(homeDetailSource == HOME_DETAIL_ANIME)
    {
        int index = homeDetailItemIndex;
        if(index < 0)
            index = 0;
        if(index > 2)
            index = 2;

        description = animeList[index].description;
    }
    else
    {
        int index = homeDetailItemIndex;
        if(index < 0)
            index = 0;
        if(index > 2)
            index = 2;

        description = movies[index].description;
    }

    if(description.length() == 0)
        description = "No description available.";

    return description;
}

int wrappedRowCount(const String &text, int charsPerRow)
{
    if(charsPerRow <= 0)
        return 1;

    if(text.length() == 0)
        return 1;

    return (text.length() + charsPerRow - 1) / charsPerRow;
}

String wrappedRowSlice(const String &text, int rowIndex, int charsPerRow)
{
    if(charsPerRow <= 0 || rowIndex < 0)
        return "";

    int start = rowIndex * charsPerRow;
    if(start >= static_cast<int>(text.length()))
        return "";

    int end = start + charsPerRow;
    if(end > static_cast<int>(text.length()))
        end = text.length();

    return text.substring(start, end);
}

void setRightLedBrightness(uint8_t duty)
{
    ledcWrite(RIGHT_LED_PWM_CHANNEL, duty);
}

void drawBatteryStatusIcon(U8G2 &oled, int x, int y, int width, int height, int batteryLevel, bool lowPowerModeOn)
{
    if(batteryLevel < 0)
        batteryLevel = 0;

    if(batteryLevel > 100)
        batteryLevel = 100;

    const int tipWidth = width / 8;
    const int bodyWidth = width - tipWidth - 1;
    const int tipHeight = height / 2;
    const int tipY = y + ((height - tipHeight) / 2);
    const int innerPadding = 2;

    oled.drawFrame(x, y, bodyWidth, height);
    oled.drawBox(x + bodyWidth, tipY, tipWidth, tipHeight);

    int innerWidth = bodyWidth - (innerPadding * 2);
    int innerHeight = height - (innerPadding * 2);
    int fillWidth = map(batteryLevel, 0, 100, 0, innerWidth);

    if(fillWidth > 0 && innerHeight > 0)
        oled.drawBox(x + innerPadding, y + innerPadding, fillWidth, innerHeight);

    if(lowPowerModeOn)
    {
        oled.drawFrame(x - 2, y - 2, bodyWidth + tipWidth + 5, height + 4);
        int boltX = x + (bodyWidth / 2) - 4;
        int boltY = y + 5;
        oled.drawPixel(boltX + 2, boltY);
        oled.drawPixel(boltX + 1, boltY + 2);
        oled.drawPixel(boltX + 4, boltY + 2);
        oled.drawPixel(boltX + 3, boltY + 4);
        oled.drawPixel(boltX + 2, boltY + 6);
    }
}

void refreshAutomaticLowPowerMode(int batteryLevel)
{
    int triggerPercent = autoLowPowerTriggerPercent();

    if(triggerPercent < 0)
    {
        lowPowerManualOverrideOff = false;
        return;
    }

    if(batteryLevel > triggerPercent)
    {
        lowPowerManualOverrideOff = false;
        return;
    }

    if(!lowPowerModeEnabled && !lowPowerManualOverrideOff)
    {
        lowPowerModeEnabled = true;
        applyLowPowerModeState();
        saveUiSettings();
    }
}

void triggerLeftLedFlashPattern(unsigned long durationMs)
{
    leftLedFlashStartedAt = millis();
    leftLedFlashUntil = leftLedFlashStartedAt + durationMs;
}

bool displayedMoviesChanged(const String previousTitles[3], const String previousDates[3])
{
    for(int i = 0; i < 3; i++)
    {
        if(previousTitles[i] != movies[i].title)
            return true;

        if(previousDates[i] != movies[i].date)
            return true;
    }

    return false;
}

bool displayedAnimeChanged(const String previousTitles[3], const String previousTimes[3], const long previousAiringAt[3])
{
    for(int i = 0; i < 3; i++)
    {
        if(previousTitles[i] != animeList[i].title)
            return true;

        if(previousTimes[i] != animeList[i].time)
            return true;

        if(previousAiringAt[i] != animeList[i].airingAt)
            return true;
    }

    return false;
}

void updateAlertOutputs(int batteryLevel)
{
    if(batteryLevel < 0)
        batteryLevel = 0;

    if(batteryLevel > 100)
        batteryLevel = 100;

    bool shouldBeep = false;

    if(timeReady() && selectedAnimeAlertAiringAt > 0)
    {
        time_t now = time(nullptr);
        time_t airingAt = selectedAnimeAlertAiringAt;
        time_t leadTime = static_cast<time_t>(buzzerLeadTimeMs() / 1000UL);

        if(ledAlertsActive() && now < airingAt && isSameLocalDay(now, airingAt))
        {
            const unsigned long pulseMs = 3000;
            const unsigned long gapMs = 60000;
            const unsigned long cycleMs = pulseMs + gapMs;
            unsigned long phase = millis() % cycleMs;

            if(phase < pulseMs)
            {
                const unsigned long halfPulseMs = pulseMs / 2;
                uint8_t duty;

                if(phase < halfPulseMs)
                    duty = map(phase, 0, halfPulseMs, 0, 255);
                else
                    duty = map(phase - halfPulseMs, 0, halfPulseMs, 255, 0);

                setRightLedBrightness(duty);
            }
            else
            {
                setRightLedBrightness(0);
            }
        }
        else
        {
            setRightLedBrightness(0);
        }

        if(now >= (airingAt - leadTime) && selectedAnimeAlertMutedAiringAt != selectedAnimeAlertAiringAt)
            shouldBeep = true;
    }

    if(shouldBeep && buzzerAlertsActive())
    {
        const unsigned long beepInterval = 350;

        if(!selectedAnimeAlertBeepOn)
        {
            selectedAnimeAlertBeepOn = true;
            selectedAnimeAlertToggleAt = millis();
            digitalWrite(BUZZER_PIN, HIGH);
        }
        else if(millis() - selectedAnimeAlertToggleAt >= beepInterval)
        {
            selectedAnimeAlertToggleAt = millis();
            selectedAnimeAlertBeepOn = !selectedAnimeAlertBeepOn;
            digitalWrite(BUZZER_PIN, selectedAnimeAlertBeepOn ? HIGH : LOW);
        }
    }
    else
    {
        selectedAnimeAlertBeepOn = false;
        digitalWrite(BUZZER_PIN, LOW);
    }

    bool leftLedOn = false;

    if(wifiConnecting && WiFi.status() != WL_CONNECTED)
    {
        leftLedOn = ((millis() / 500) % 2) == 0;
    }
    else if(WiFi.status() == WL_CONNECTED && (!moviesLoaded || !animeLoaded))
    {
        leftLedOn = ((millis() / 700) % 2) == 0;
    }
    else if(wifiConnectionFailed)
    {
        unsigned long phase = millis() % 3000;
        leftLedOn = phase < 180 || (phase >= 360 && phase < 540);
    }
    else if(movieFetchFailed || animeFetchFailed)
    {
        unsigned long phase = millis() % 2600;
        leftLedOn = phase < 600;
    }
    else if(leftLedFlashUntil > millis())
    {
        unsigned long phase = millis() - leftLedFlashStartedAt;
        leftLedOn = phase < 90 || (phase >= 180 && phase < 270);
    }
    else if(ledAlertsActive() && timeReady() && selectedAnimeAlertAiringAt > 0)
    {
        time_t now = time(nullptr);
        time_t airingAt = selectedAnimeAlertAiringAt;

        if(now < airingAt && isSameLocalDay(now, airingAt) && (airingAt - now) <= 300)
            leftLedOn = ((millis() / 900) % 2) == 0;
    }

    digitalWrite(LEFT_LED_PIN, leftLedOn ? HIGH : LOW);

    if(!ledAlertsActive())
        setRightLedBrightness(0);

    if(!ledAlertsActive() || !(timeReady() && selectedAnimeAlertAiringAt > 0 && isSameLocalDay(time(nullptr), selectedAnimeAlertAiringAt) && time(nullptr) < selectedAnimeAlertAiringAt))
        setRightLedBrightness(0);
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

void drawWifiMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = WIFI_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 84;
    const int targetHeight = 42;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 2)
        scale = 2;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = WIFI_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
}

void drawSettingsMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = SETTINGS_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 84;
    const int targetHeight = 42;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 2)
        scale = 2;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = SETTINGS_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
}

void drawAboutMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = ABOUT_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 84;
    const int targetHeight = 42;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 2)
        scale = 2;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = ABOUT_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
}

void drawListMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = LIST_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 84;
    const int targetHeight = 42;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 2)
        scale = 2;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = LIST_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
}

void drawRebootMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = REBOOT_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 84;
    const int targetHeight = 42;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 2)
        scale = 2;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = REBOOT_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
}

void drawBatteryMenuIcon(U8G2 &oled)
{
    const int bitmapWidth = 32;
    const int bitmapHeight = 32;

    int minX = bitmapWidth;
    int minY = bitmapHeight;
    int maxX = -1;
    int maxY = -1;

    for(int y = 0; y < bitmapHeight; y++)
    {
        for(int x = 0; x < bitmapWidth; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = BATTERY_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                if(x < minX) minX = x;
                if(y < minY) minY = y;
                if(x > maxX) maxX = x;
                if(y > maxY) maxY = y;
            }
        }
    }

    if(maxX < minX || maxY < minY)
        return;

    int glyphWidth = maxX - minX + 1;
    int glyphHeight = maxY - minY + 1;

    const int targetWidth = 48;
    const int targetHeight = 24;

    int scaleX = targetWidth / glyphWidth;
    int scaleY = targetHeight / glyphHeight;
    int scale = scaleX < scaleY ? scaleX : scaleY;

    if(scale < 1)
        scale = 1;
    if(scale > 1)
        scale = 1;

    int drawWidth = glyphWidth * scale;
    int drawHeight = glyphHeight * scale;

    int startX = 64 - (drawWidth / 2);
    int startY = 36 - (drawHeight / 2);

    for(int y = minY; y <= maxY; y++)
    {
        for(int x = minX; x <= maxX; x++)
        {
            int byteIndex = y * (bitmapWidth / 8) + (x / 8);
            uint8_t value = BATTERY_ICON_BITMAP[byteIndex];

            if((value >> (7 - (x % 8))) & 0x01)
            {
                int px = startX + (x - minX) * scale;
                int py = startY + (y - minY) * scale;

                if(scale == 1)
                    oled.drawPixel(px, py);
                else
                    oled.drawBox(px, py, scale, scale);
            }
        }
    }
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
    String headerTime = currentTimeLabel();
    drawHeader(display, batteryPercent(), lowPowerModeEnabled, headerTime.c_str(), "");

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
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(LEFT_LED_PIN, OUTPUT);
        pinMode(RIGHT_LED_PIN, OUTPUT);
    ledcSetup(RIGHT_LED_PWM_CHANNEL, RIGHT_LED_PWM_FREQUENCY, RIGHT_LED_PWM_RESOLUTION);
    ledcAttachPin(RIGHT_LED_PIN, RIGHT_LED_PWM_CHANNEL);
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LEFT_LED_PIN, LOW);
    setRightLedBrightness(0);

    displayInit();

    loadUiSettings();
    applyDisplayBrightness();
    applyLowPowerModeState();
    lastInteractionAt = millis();

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
loadSelectedMovieTitles();
}


void loop()
{
    buttonsUpdate();

    bool okPress = okPressed();
    bool okLongPress = okLongPressed();
    bool leftPress = leftPressed();
    bool rightPress = rightPressed();
    int batteryLevel = batteryPercent();

    refreshAutomaticLowPowerMode(batteryLevel);

    if(millis() < wakeIgnoreUntil)
    {
        okPress = false;
        okLongPress = false;
        leftPress = false;
        rightPress = false;
    }

    bool anyInteraction = okPress || okLongPress || leftPress || rightPress;

    if(lowPowerModeEnabled)
    {
        if(displaySleeping)
        {
            if(okPress)
            {
                displaySleeping = false;
                display.setPowerSave(0);
                applyDisplayBrightness();
                lastInteractionAt = millis();
            }

            okPress = false;
            okLongPress = false;
            leftPress = false;
            rightPress = false;
        }
        else
        {
            if(anyInteraction)
                lastInteractionAt = millis();

            unsigned long timeoutMs = screenTimeoutMs();
            if(timeoutMs > 0 && millis() - lastInteractionAt >= timeoutMs)
            {
                displaySleeping = true;
                enterLowPowerSleep();
                okPress = false;
                okLongPress = false;
                leftPress = false;
                rightPress = false;
            }
        }
    }
    else if(displaySleeping)
    {
        displaySleeping = false;
        display.setPowerSave(0);
        applyDisplayBrightness();
    }

    display.clearBuffer();
    if(WiFi.status() == WL_CONNECTED)
{
    wifiConnecting = false;
    wifiConnectionFailed = false;
    wifiHasSavedCredentials = true;

    currentSSID = WiFi.SSID();
}

    String headerTime = currentTimeLabel();
    drawHeader(display, batteryLevel, lowPowerModeEnabled, headerTime.c_str(), currentSSID.c_str());

    if(currentScreen == SCREEN_HOME && (leftPress || rightPress))
    {
        silenceSelectedAnimeAlert();

        if(!homeCursorActive)
        {
            if(leftPress && !rightPress)
                homeCursorIndex = HOME_CURSOR_SLOT_COUNT - 1;
            else
                homeCursorIndex = 0;

            homeCursorActive = true;
        }
        else if(leftPress && !rightPress)
        {
            homeCursorIndex = (homeCursorIndex + HOME_CURSOR_SLOT_COUNT - 1) % HOME_CURSOR_SLOT_COUNT;
        }
        else if(rightPress && !leftPress)
        {
            homeCursorIndex = (homeCursorIndex + 1) % HOME_CURSOR_SLOT_COUNT;
        }
    }

    if(wifiConnecting && WiFi.status() != WL_CONNECTED && millis() - wifiConnectStart > 20000)
    {
        wifiConnecting = false;
        wifiConnectionFailed = true;
    }

    // Prioritize home actions before any network fetch work.
    if(currentScreen == SCREEN_HOME && okPress)
    {
        if(homeCursorActive)
            openHomeCursorDetails();
        else
            currentScreen = SCREEN_MENU;

        okPress = false;
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

        // Keep title refresh timers active on all screens while connected.
        {
            unsigned long movieInterval = moviesLoaded ? refreshIntervalMs() : LOAD_RETRY_INTERVAL;
            if(now - movieUpdateTimer > movieInterval)
            {
                bool hadMovieRows = moviesLoaded;
                String previousMovieTitles[3];
                String previousMovieDates[3];
                for(int i = 0; i < 3; i++)
                {
                    previousMovieTitles[i] = movies[i].title;
                    previousMovieDates[i] = movies[i].date;
                }

                bool movieFetchOk = fetchMovies();
                movieUpdateTimer = now;

                if(movieFetchOk)
                {
                    movieFetchFailed = false;

                    if(hadMovieRows && displayedMoviesChanged(previousMovieTitles, previousMovieDates))
                        triggerLeftLedFlashPattern(320);

                    moviesLoaded = true;
                }
                else
                {
                    movieFetchFailed = true;
                }
            }

            unsigned long animeInterval = animeLoaded ? refreshIntervalMs() : LOAD_RETRY_INTERVAL;
            if(now - animeUpdateTimer > animeInterval)
            {
                long previousSelectedAnimeAlertAiringAt = selectedAnimeAlertAiringAt;
                bool hadAnimeRows = animeLoaded;
                String previousAnimeTitles[3];
                String previousAnimeTimes[3];
                long previousAnimeAiringAt[3];
                for(int i = 0; i < 3; i++)
                {
                    previousAnimeTitles[i] = animeList[i].title;
                    previousAnimeTimes[i] = animeList[i].time;
                    previousAnimeAiringAt[i] = animeList[i].airingAt;
                }

                bool animeFetchOk = fetchAnime();

                if(animeFetchOk)
                {
                    animeFetchFailed = false;

                    if(selectedAnimeTitleCount > 0 && animeTitleIsSelected(animeList[0].title))
                        selectedAnimeAlertAiringAt = animeList[0].airingAt;
                    else
                        selectedAnimeAlertAiringAt = 0;

                    if(hadAnimeRows && displayedAnimeChanged(previousAnimeTitles, previousAnimeTimes, previousAnimeAiringAt))
                        triggerLeftLedFlashPattern(320);

                    if(selectedAnimeAlertAiringAt != previousSelectedAnimeAlertAiringAt)
                    {
                        selectedAnimeAlertMutedAiringAt = 0;
                        selectedAnimeAlertBeepOn = false;
                        digitalWrite(BUZZER_PIN, LOW);
                    }

                    animeLoaded = true;
                }
                else
                {
                    animeFetchFailed = true;
                }

                animeUpdateTimer = now;

                delay(500);
            }
        }

        if(currentScreen == SCREEN_WIFI_CONNECTING)
            currentScreen = SCREEN_HOME;
    }

    updateAlertOutputs(batteryLevel);

    if(currentScreen == SCREEN_HOME && okLongPress && homeCursorActive)
    {
        homeCursorActive = false;
        okLongPress = false;
    }

    if(currentScreen == SCREEN_TITLE_DETAILS && okLongPress)
    {
        currentScreen = SCREEN_HOME;
        okLongPress = false;
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
                selectionMode = SELECTION_MODE_ANIME;
                currentScreen = SCREEN_SETTINGS_ANIME_ACTION_MENU;
                break;

            case SCREEN_SETTINGS_MOVIES_COMING:
                selectionMode = SELECTION_MODE_MOVIES;
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

            case SCREEN_SETTINGS_ANIME_ACTION_MENU:
                currentScreen = selectionMode == SELECTION_MODE_MOVIES ? SCREEN_SETTINGS_MOVIES_COMING : SCREEN_SETTINGS_ANIME_LIST;
                break;

            case SCREEN_SETTINGS:
                currentScreen = SCREEN_HOME;
                break;

            case SCREEN_SETTINGS_EMPTY:
                currentScreen = SCREEN_MENU;
                break;

            case SCREEN_REBOOT_CONFIRM:
                currentScreen = SCREEN_MENU;
                break;

            case SCREEN_BATTERY_INFO:
                currentScreen = SCREEN_MENU;
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
    bool cursorBlinkVisible = ((millis() / 350) % 2) == 0;


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
        if(!moviesLoaded || !animeLoaded)
        {
            display.drawStr(42, 34, "Loading...");
        }
        else
        {
            for(int i=0;i<3;i++)
            {
                int y = 20 + i*14;

                bool hideAnimeTitle = homeCursorActive && homeCursorIndex == i && !cursorBlinkVisible;
                bool hideMovieTitle = homeCursorActive && homeCursorIndex == (i + 3) && !cursorBlinkVisible;

                // Anime left
                if(!hideAnimeTitle)
                    drawScrollingText(display, animeX, y, titleWidth, animeList[i].title, scrollNow);

                display.drawStr(
                    animeX,
                    y+6,
                    trimDisplayText(animeList[i].time, 12).c_str()
                );


                // Movie title
                if(!hideMovieTitle)
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
}

if(currentScreen == SCREEN_TITLE_DETAILS)
{
    display.setFont(u8g2_font_4x6_tr);

    String description = detailDescriptionForScreen();
    int totalRows = wrappedRowCount(description, DETAIL_CHARS_PER_ROW);
    int maxScrollRow = totalRows > DETAIL_VISIBLE_ROWS ? (totalRows - DETAIL_VISIBLE_ROWS) : 0;

    unsigned long autoReturnMs = detailAutoReturnMs();
    if(autoReturnMs > 0 && millis() - homeDetailOpenedAt >= autoReturnMs)
    {
        currentScreen = SCREEN_HOME;
        homeDetailScrollRow = 0;
    }

    if(leftPress && homeDetailScrollRow > 0)
        homeDetailScrollRow--;

    if(rightPress && homeDetailScrollRow < maxScrollRow)
        homeDetailScrollRow++;

    for(int row = 0; row < DETAIL_VISIBLE_ROWS; row++)
    {
        int contentRow = homeDetailScrollRow + row;
        if(contentRow >= totalRows)
            break;

        String line = wrappedRowSlice(description, contentRow, DETAIL_CHARS_PER_ROW);
        int y = 18 + (row * 6);
        display.drawStr(1, y, line.c_str());
    }
}


   if(currentScreen == SCREEN_MENU)
{
    // Navigation
    if(leftPress)
    {
        if(selectedMenu == MAIN_MENU_WIFI)
            selectedMenu = MAIN_MENU_REBOOT;
        else
            selectedMenu = (MainMenuItem)(selectedMenu - 1);
    }

    if(rightPress)
    {
        if(selectedMenu == MAIN_MENU_REBOOT)
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
                selectedDeviceSetting = DEVICE_SETTING_BRIGHTNESS;
                settingsMenuTop = 0;
                currentScreen = SCREEN_SETTINGS_EMPTY;
                okPress = false;
                break;

            case MAIN_MENU_ABOUT:
                aboutInfoLineIndex = 0;
                currentScreen = SCREEN_ABOUT;
                okPress = false;
                break;

            case MAIN_MENU_LIST:
                selectedSettingsMenu = SETTINGS_DEFAULT;
                currentScreen = SCREEN_SETTINGS;
                okPress = false;
                break;

            case MAIN_MENU_BATTERY:
                currentScreen = SCREEN_BATTERY_INFO;
                okPress = false;
                break;

            case MAIN_MENU_REBOOT:
                rebootActionItem = REBOOT_ACTION_REBOOT;
                currentScreen = SCREEN_REBOOT_CONFIRM;
                okPress = false;
                break;

            default:
                break;
        }
    }

    if(selectedMenu == MAIN_MENU_WIFI)
    {
        drawWifiMenuIcon(display);
    }
    else if(selectedMenu == MAIN_MENU_SETTINGS)
    {
        drawSettingsMenuIcon(display);
    }
    else if(selectedMenu == MAIN_MENU_ABOUT)
    {
        drawAboutMenuIcon(display);
    }
    else if(selectedMenu == MAIN_MENU_LIST)
    {
        drawListMenuIcon(display);
    }
    else if(selectedMenu == MAIN_MENU_BATTERY)
    {
        drawBatteryMenuIcon(display);
    }
    else
    {
        drawRebootMenuIcon(display);
    }

}

if(currentScreen == SCREEN_REBOOT_CONFIRM)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress || rightPress)
        rebootActionItem = rebootActionItem == REBOOT_ACTION_REBOOT ? REBOOT_ACTION_POWER_OFF : REBOOT_ACTION_REBOOT;

    if(okPress)
    {
        if(rebootActionItem == REBOOT_ACTION_REBOOT)
            ESP.restart();
        else
            enterMenuPowerOffSleep();

        okPress = false;
    }

    display.drawStr(16, 40, rebootActionItem == REBOOT_ACTION_REBOOT ? "> Reboot" : "  Reboot");
    display.drawStr(60, 40, rebootActionItem == REBOOT_ACTION_POWER_OFF ? "> Power off" : "  Power off");
}

if(currentScreen == SCREEN_SETTINGS_EMPTY)
{
    display.setFont(u8g2_font_4x6_tr);

    if(leftPress)
    {
        int currentItem = static_cast<int>(selectedDeviceSetting);
        currentItem = (currentItem - 1 + DEVICE_SETTINGS_COUNT) % DEVICE_SETTINGS_COUNT;
        selectedDeviceSetting = static_cast<DeviceSettingsItem>(currentItem);

        if(selectedDeviceSetting < settingsMenuTop)
            settingsMenuTop = (selectedDeviceSetting >= SETTINGS_VISIBLE_ITEMS - 1) ? selectedDeviceSetting - SETTINGS_VISIBLE_ITEMS + 1 : 0;
    }

    if(rightPress)
    {
        int currentItem = static_cast<int>(selectedDeviceSetting);
        currentItem = (currentItem + 1) % DEVICE_SETTINGS_COUNT;
        selectedDeviceSetting = static_cast<DeviceSettingsItem>(currentItem);

        if(selectedDeviceSetting < settingsMenuTop)
            settingsMenuTop = 0;

        if(selectedDeviceSetting >= settingsMenuTop + SETTINGS_VISIBLE_ITEMS)
            settingsMenuTop = selectedDeviceSetting - SETTINGS_VISIBLE_ITEMS + 1;
    }

    if(okPress)
    {
        changeDeviceSettingValue(selectedDeviceSetting);
        okPress = false;
    }

    for(int i = 0; i < SETTINGS_VISIBLE_ITEMS; i++)
    {
        int index = settingsMenuTop + i;
        if(index >= DEVICE_SETTINGS_COUNT)
            break;

        int y = 20 + (i * 9);
        DeviceSettingsItem item = static_cast<DeviceSettingsItem>(index);
        String line = String(selectedDeviceSetting == item ? "> " : "  ") + deviceSettingLabel(item) + ": " + deviceSettingValue(item);
        display.drawStr(1, y, line.c_str());
    }
}

if(currentScreen == SCREEN_BATTERY_INFO)
{
    float batteryVolts = batteryVoltage();
    char voltageInfo[20];
    char percentInfo[20];

    snprintf(voltageInfo, sizeof(voltageInfo), "Voltage: %.2fV", batteryVolts);
    snprintf(percentInfo, sizeof(percentInfo), "Charge: %d%%", batteryLevel);

    display.setFont(u8g2_font_5x7_tr);
    drawBatteryStatusIcon(display, 34, 18, 60, 24, batteryLevel, lowPowerModeEnabled);
    display.drawStr(26, 50, voltageInfo);
    display.drawStr(29, 60, percentInfo);
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
                clearSelectedMovieTitles();
                animeLoaded = false;
                moviesLoaded = false;
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

    display.drawStr(10, 28, selectedSettingsMenu == SETTINGS_DEFAULT ? "> Default" : "  Default");
    display.drawStr(10, 40, selectedSettingsMenu == SETTINGS_SELECT_TITLE ? "> Select Title" : "  Select Title");
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
                fetchSeasonMovieChoices();
                syncSelectedMovieChoices();
                seasonMovieCursor = 0;
                seasonMovieTop = 0;
                currentScreen = SCREEN_SETTINGS_MOVIES_COMING;
                okPress = false;
                break;

            case TITLE_SOURCE_ANIME:
                selectionMode = SELECTION_MODE_ANIME;
                fetchSeasonAnimeChoices();
                syncSelectedAnimeChoices();
                seasonAnimeCursor = 0;
                seasonAnimeTop = 0;
                currentScreen = SCREEN_SETTINGS_ANIME_LIST;
                okPress = false;
                break;
        }
    }

    display.drawStr(10, 28, selectedTitleSource == TITLE_SOURCE_MOVIES ? "> Movies" : "  Movies");
    display.drawStr(10, 40, selectedTitleSource == TITLE_SOURCE_ANIME ? "> Anime" : "  Anime");
}

if(currentScreen == SCREEN_SETTINGS_MOVIES_COMING)
{
    display.setFont(u8g2_font_5x7_tr);

    if(leftPress && seasonMovieCount > 0)
    {
        if(seasonMovieCursor > 0)
            seasonMovieCursor--;

        if(seasonMovieCursor < seasonMovieTop)
            seasonMovieTop--;
    }

    if(rightPress && seasonMovieCount > 0)
    {
        if(seasonMovieCursor < seasonMovieCount - 1)
        {
            seasonMovieCursor++;

            if(seasonMovieCursor > seasonMovieTop + (VISIBLE_MOVIE_CHOICES - 1))
                seasonMovieTop++;
        }
    }

    if(okPress && seasonMovieCount > 0)
    {
        seasonMovieChoices[seasonMovieCursor].selected = !seasonMovieChoices[seasonMovieCursor].selected;
        okPress = false;
    }

    if(seasonMovieCount == 0)
    {
        display.drawStr(14, 36, "No movies this year");
    }
    else
    {
        for(int i = 0; i < VISIBLE_MOVIE_CHOICES; i++)
        {
            int index = seasonMovieTop + i;

            if(index >= seasonMovieCount)
                break;

            int y = 18 + (i * 7);
            display.drawStr(2, y, index == seasonMovieCursor ? ">" : " ");
            display.drawStr(8, y, seasonMovieChoices[index].selected ? "*" : " ");
            drawScrollingText(display, 14, y, 78, seasonMovieChoices[index].title, millis());
            display.drawStr(94, y, trimDisplayText(seasonMovieChoices[index].date, 10).c_str());
        }
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
                if(selectionMode == SELECTION_MODE_MOVIES)
                {
                    rebuildSelectedMovieTitlesFromChoices();
                    saveSelectedMovieTitles();
                    moviesLoaded = false;
                }
                else
                {
                    rebuildSelectedAnimeTitlesFromChoices();
                    saveSelectedAnimeTitles();
                    animeLoaded = false;
                }
                selectionDoneUntil = millis() + 700;
                currentScreen = SCREEN_SETTINGS_DONE;
                okPress = false;
                break;

            case ANIME_ACTION_CANCEL:
                currentScreen = SCREEN_MENU;
                okPress = false;
                break;
        }
    }

    display.drawStr(14, 28, selectedAnimeAction == ANIME_ACTION_DONE ? "> Done" : "  Done");
    display.drawStr(14, 40, selectedAnimeAction == ANIME_ACTION_CANCEL ? "> Cancel" : "  Cancel");
}

if(currentScreen == SCREEN_SETTINGS_DONE)
{
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(24, 28, "Done");

    if(millis() > selectionDoneUntil)
        currentScreen = SCREEN_HOME;
}

if(currentScreen == SCREEN_ABOUT)
{
    if(leftPress)
    {
        aboutInfoLineIndex--;
        if(aboutInfoLineIndex < 0)
            aboutInfoLineIndex = ABOUT_INFO_LINE_COUNT - 1;
    }

    if(rightPress)
    {
        aboutInfoLineIndex++;
        if(aboutInfoLineIndex >= ABOUT_INFO_LINE_COUNT)
            aboutInfoLineIndex = 0;
    }

    display.drawXBM(44, 18, 40, 40, ABOUT_SCREEN_BITMAP);

    display.setFont(u8g2_font_4x6_tr);
    const char *aboutLine = ABOUT_INFO_LINES[aboutInfoLineIndex];
    int aboutTextWidth = display.getStrWidth(aboutLine);
    int aboutTextX = (128 - aboutTextWidth) / 2;
    if(aboutTextX < 0)
        aboutTextX = 0;

    display.drawStr(aboutTextX, 63, aboutLine);
}


    display.sendBuffer();

    delay(10);
}