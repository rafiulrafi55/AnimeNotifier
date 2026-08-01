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
        return false;

    for(int i = 0; i < selectedAnimeTitleCount; i++)
    {
        if(title == selectedAnimeTitles[i])
            return true;
    }

    return false;
}

static String normalizeDescription(const String &raw)
{
    if(raw.length() == 0)
        return "No description available.";

    String cleaned;
    cleaned.reserve(raw.length());
    bool previousWasSpace = false;

    for(size_t i = 0; i < raw.length(); i++)
    {
        char c = raw[i];
        bool isSpace = c == '\n' || c == '\r' || c == '\t' || c == ' ';

        if(isSpace)
        {
            if(!previousWasSpace)
            {
                cleaned += ' ';
                previousWasSpace = true;
            }
        }
        else
        {
            cleaned += c;
            previousWasSpace = false;
        }
    }

    cleaned.trim();
    return cleaned.length() > 0 ? cleaned : "No description available.";
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

bool fetchAnime()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Required for HTTPS on AniList GraphQL API

    HTTPClient http;
    http.begin(client, "https://graphql.anilist.co");
    http.addHeader("Content-Type", "application/json");

        long now = time(nullptr);

    String query = R"(
    {
                        Page(page:PAGE_PLACEHOLDER, perPage:25)
      {
        pageInfo { hasNextPage }
                    airingSchedules(sort: TIME, airingAt_greater: NOW_PLACEHOLDER)
        {
                    airingAt
                    episode
                                        media {
                                                title { romaji }
                                                description(asHtml:false)
                                        }
        }
      }
    }
    )";

    struct AnimeRow
    {
        String title;
        String time;
        long airingAt;
        String description;
    };

    bool useSelectedPriority = selectedAnimeTitleCount > 0;
    AnimeRow selectedRow;
    AnimeRow generalRows[3];
    int generalCount = 0;
    bool hasSelectedRow = false;
    bool hadAnyApiSuccess = false;
    int targetGeneralCount = useSelectedPriority ? 2 : 3;

    for(int page = 1; page <= 20; page++)
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
            if(hadAnyApiSuccess)
                break;

            http.end();
            return false;
        }

        hadAnyApiSuccess = true;

        String payload = http.getString();

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            if(hadAnyApiSuccess)
                break;

            http.end();
            return false;
        }

        JsonObject pageInfo = doc["data"]["Page"]["pageInfo"];
        JsonArray results = doc["data"]["Page"]["airingSchedules"];

        if(results.size() == 0)
            break;

        for (JsonObject item : results)
        {
            if((useSelectedPriority && hasSelectedRow && generalCount >= targetGeneralCount) || (!useSelectedPriority && generalCount >= targetGeneralCount))
                break;

            String title = item["media"]["title"]["romaji"] | "Unknown Title";
            long airing = item["airingAt"] | 0;
            String timeLabel = formatAnimeTimeLabel(airing);
            String description = normalizeDescription(item["media"]["description"].as<String>());

            if(isSelectedAnimeTitle(title))
            {
                if(!hasSelectedRow)
                {
                    selectedRow.title = title;
                    selectedRow.time = timeLabel;
                    selectedRow.airingAt = airing;
                    selectedRow.description = description;
                    hasSelectedRow = true;
                }

                continue;
            }

            bool alreadyAdded = false;
            if(hasSelectedRow && selectedRow.title == title)
                alreadyAdded = true;

            for(int i = 0; i < generalCount; i++)
            {
                if(generalRows[i].title == title)
                {
                    alreadyAdded = true;
                    break;
                }
            }

            if(alreadyAdded)
                continue;

            if(generalCount < targetGeneralCount)
            {
                generalRows[generalCount].title = title;
                generalRows[generalCount].time = timeLabel;
                generalRows[generalCount].airingAt = airing;
                generalRows[generalCount].description = description;
                generalCount++;
            }
        }

        if(!pageInfo["hasNextPage"].as<bool>())
            break;
    }

    for(int i = 0; i < 3; i++)
    {
        animeList[i].title = "";
        animeList[i].time = "";
        animeList[i].airingAt = 0;
        animeList[i].description = "";
    }

    int outputIndex = 0;

    if(useSelectedPriority && hasSelectedRow)
    {
        animeList[0].title = selectedRow.title;
        animeList[0].time = selectedRow.time;
        animeList[0].airingAt = selectedRow.airingAt;
        animeList[0].description = selectedRow.description;
        outputIndex = 1;
    }

    for(int i = 0; i < generalCount && outputIndex < 3; i++)
    {
        animeList[outputIndex].title = generalRows[i].title;
        animeList[outputIndex].time = generalRows[i].time;
        animeList[outputIndex].airingAt = generalRows[i].airingAt;
        animeList[outputIndex].description = generalRows[i].description;
        outputIndex++;
    }

    http.end();
    return outputIndex > 0;
}

void fetchSeasonAnimeChoices()
{
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

        for(int page = 1; page <= 100 && seasonAnimeCount < MAX_ANIME_CHOICES; page++)
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