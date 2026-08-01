#include "anilist.h"
#include "anime.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

static String currentSeasonName()
{
    time_t nowRaw = time(nullptr);
    struct tm nowTime;
    localtime_r(&nowRaw, &nowTime);

    int month = nowTime.tm_mon + 1;

    if(month == 12 || month <= 2)
        return "WINTER";

    if(month <= 5)
        return "SPRING";

    if(month <= 8)
        return "SUMMER";

    return "FALL";
}

static int currentSeasonYear()
{
    time_t nowRaw = time(nullptr);
    struct tm nowTime;
    localtime_r(&nowRaw, &nowTime);

    return nowTime.tm_year + 1900;
}

static bool isSelectedAnimeTitle(const String &title)
{
    if(selectedAnimeTitleCount == 0)
        return true;

    for(int i = 0; i < selectedAnimeTitleCount; i++)
    {
        if(title == selectedAnimeTitles[i])
            return true;
    }

    return false;
}

String formatAnimeTime(long timestamp)
{
    if (timestamp <= 0)
        return "--:--";

    time_t raw = timestamp;
    struct tm timeinfo;

    // Uses ESP32 system timezone configured via configTime()
    localtime_r(&raw, &timeinfo);

    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    return String(buffer);
}

String formatAnimeTimeLabel(long timestamp)
{
    if (timestamp <= 0)
        return "--:--";

    time_t raw = timestamp;
    struct tm airingTime;
    localtime_r(&raw, &airingTime);

    time_t nowRaw = time(nullptr);
    struct tm nowTime;
    localtime_r(&nowRaw, &nowTime);

    char timeBuffer[10];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", airingTime.tm_hour, airingTime.tm_min);

    int dayDelta = (airingTime.tm_year - nowTime.tm_year) * 365 + (airingTime.tm_yday - nowTime.tm_yday);

    if(dayDelta == 0)
        return String(timeBuffer) + " Today";

    if(dayDelta == 1)
        return String(timeBuffer) + " Tomorrow";

    char dateBuffer[16];
    strftime(dateBuffer, sizeof(dateBuffer), "%d %b", &airingTime);
    return String(timeBuffer) + " " + dateBuffer;
}

void fetchAnime()
{
    Serial.println("Fetching anime data...");

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("No WiFi connection, aborting.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Required for HTTPS on AniList GraphQL API

    HTTPClient http;
    http.begin(client, "https://graphql.anilist.co");
    http.addHeader("Content-Type", "application/json");

        long now = time(nullptr);

    String query = R"(
    {
            Page(page:PAGE_PLACEHOLDER, perPage:100)
      {
        pageInfo { hasNextPage }
                    airingSchedules(sort: TIME, airingAt_greater: NOW_PLACEHOLDER)
        {
                    airingAt
                    episode
                    media { title { romaji } }
        }
      }
    }
    )";

    int index = 0;

    for(int page = 1; page <= 20 && index < 3; page++)
    {
        JsonDocument request;
        String pageQuery = query;
        pageQuery.replace("NOW_PLACEHOLDER", String(now));
        pageQuery.replace("PAGE_PLACEHOLDER", String(page));
        request["query"] = pageQuery;

        String body;
        serializeJson(request, body);

        int code = http.POST(body);

        if (code != 200)
        {
            Serial.printf("HTTP Error Code: %d\n", code);
            http.end();
            return;
        }

        String payload = http.getString();

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            Serial.printf("JSON Error: %s\n", error.c_str());
            http.end();
            return;
        }

        JsonObject pageInfo = doc["data"]["Page"]["pageInfo"];
        JsonArray results = doc["data"]["Page"]["airingSchedules"];

        if(results.size() == 0)
            break;

        for (JsonObject item : results)
        {
            if (index >= 3)
                break;

            String title = item["media"]["title"]["romaji"] | "Unknown Title";

            if(!isSelectedAnimeTitle(title))
                continue;

            bool alreadyAdded = false;
            for(int i = 0; i < index; i++)
            {
                if(animeList[i].title == title)
                {
                    alreadyAdded = true;
                    break;
                }
            }

            if(alreadyAdded)
                continue;

            animeList[index].title = title;

            long airing = item["airingAt"] | 0;

            animeList[index].time = formatAnimeTimeLabel(airing);

            Serial.printf("Index %d: Title=%s, Time=%s\n", index, animeList[index].title.c_str(), animeList[index].time.c_str());
            index++;
        }

        if(!pageInfo["hasNextPage"].as<bool>())
            break;
    }

    while (index < 3)
    {
        animeList[index].title = "";
        animeList[index].time = "";
        index++;
    }

    http.end();
    Serial.println("Finished fetching anime data.");
}

void fetchSeasonAnimeChoices()
{
        Serial.println("Fetching current season anime choices...");

        if(WiFi.status() != WL_CONNECTED)
                return;

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient http;
        http.begin(client, "https://graphql.anilist.co");
        http.addHeader("Content-Type", "application/json");

        String query = R"(
        {
            Page(page:PAGE_PLACEHOLDER, perPage:100)
            {
                pageInfo { hasNextPage }
                media(
                    type: ANIME,
                    format: TV,
                    sort: TITLE_ROMAJI,
                    status_in: [RELEASING, NOT_YET_RELEASED]
                )
                {
                    id
                    title { romaji }
                }
            }
        }
        )";
        seasonAnimeCount = 0;

        for(int page = 1; page <= 50 && seasonAnimeCount < MAX_ANIME_CHOICES; page++)
        {
            JsonDocument request;
            String pageQuery = query;
            pageQuery.replace("PAGE_PLACEHOLDER", String(page));
            request["query"] = pageQuery;

            String body;
            serializeJson(request, body);

            int code = http.POST(body);
            if(code != 200)
            {
                http.end();
                return;
            }

            String payload = http.getString();
            JsonDocument doc;

            if(deserializeJson(doc, payload))
            {
                http.end();
                return;
            }

            JsonObject pageInfo = doc["data"]["Page"]["pageInfo"];
            JsonArray results = doc["data"]["Page"]["media"];

            if(results.size() == 0)
                break;

            for(JsonObject item : results)
            {
                if(seasonAnimeCount >= MAX_ANIME_CHOICES)
                    break;

                seasonAnimeChoices[seasonAnimeCount].id = item["id"] | 0;
                seasonAnimeChoices[seasonAnimeCount].title = item["title"]["romaji"] | "Unknown Title";
                seasonAnimeChoices[seasonAnimeCount].selected = isSelectedAnimeTitle(seasonAnimeChoices[seasonAnimeCount].title);

                seasonAnimeCount++;
            }

            if(!pageInfo["hasNextPage"].as<bool>())
                break;
        }

        http.end();
}