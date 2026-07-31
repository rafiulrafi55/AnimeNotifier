#include "anilist.h"
#include "anime.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

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

    String query = R"(
    {
      Page(page:1, perPage:3)
      {
        media(
          type: ANIME,
          status: RELEASING,
          sort: TIME_REMAINING_ASC
        )
        {
          title { romaji }
          nextAiringEpisode { airingAt }
        }
      }
    }
    )";

    JsonDocument request;
    request["query"] = query;

    String body;
    serializeJson(request, body);

    Serial.println("Sending GraphQL request body:");
    Serial.println(body);

    int code = http.POST(body);

    if (code != 200)
    {
        Serial.printf("HTTP Error Code: %d\n", code);
        http.end();
        return;
    }

    String payload = http.getString();

    Serial.println("Received payload:");
    Serial.println(payload);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.printf("JSON Error: %s\n", error.c_str());
        http.end();
        return;
    }

    Serial.println("Parsing results...");

    JsonArray results = doc["data"]["Page"]["media"];
    int index = 0;

    for (JsonObject item : results)
    {
        if (index >= 3)
            break;

        animeList[index].title = item["title"]["romaji"] | "Unknown Title";

        long airing = 0;
        if (item["nextAiringEpisode"] && !item["nextAiringEpisode"]["airingAt"].isNull())
        {
            airing = item["nextAiringEpisode"]["airingAt"].as<long>();
        }

        animeList[index].time = formatAnimeTime(airing);

        Serial.printf("Index %d: Title=%s, Time=%s\n", index, animeList[index].title.c_str(), animeList[index].time.c_str());
        index++;
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