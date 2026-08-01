#include "tmdb.h"
#include "secrets.h"
#include "time_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


Movie movies[3];
bool isFutureDate(String date)
{
    if(date.length() < 10)
        return false;


    int year = date.substring(0,4).toInt();
    int month = date.substring(5,7).toInt();
    int day = date.substring(8,10).toInt();


String today = currentDate();

int currentYear =
today.substring(0,4).toInt();

int currentMonth =
today.substring(5,7).toInt();

int currentDay =
today.substring(8,10).toInt();


    if(year > currentYear)
        return true;

    if(year == currentYear && month > currentMonth)
        return true;

    if(year == currentYear && month == currentMonth && day >= currentDay)
        return true;


    return false;
}

// Convert YYYY-MM-DD to DD Mon
String formatDate(String date)
{
    if(date.length() < 10)
        return date;


    String month;

    String m = date.substring(5,7);


    if(m=="01") month="Jan";
    else if(m=="02") month="Feb";
    else if(m=="03") month="Mar";
    else if(m=="04") month="Apr";
    else if(m=="05") month="May";
    else if(m=="06") month="Jun";
    else if(m=="07") month="Jul";
    else if(m=="08") month="Aug";
    else if(m=="09") month="Sep";
    else if(m=="10") month="Oct";
    else if(m=="11") month="Nov";
    else if(m=="12") month="Dec";


    return date.substring(8,10) + " " + month;
}



void fetchMovies()
{
    if(WiFi.status() != WL_CONNECTED)
        return;


    String url =
    "https://api.themoviedb.org/3/movie/upcoming?api_key="
    + String(TMDB_API_KEY)
    + "&language=en-US&page=1";


    HTTPClient http;

    http.begin(url);


    int code = http.GET();


    if(code == 200)
    {
        String payload = http.getString();


        DynamicJsonDocument doc(12000);


        if(deserializeJson(doc, payload))
        {
            http.end();
            return;
        }


        JsonArray results = doc["results"];


        int index = 0;


        for(JsonObject item : results)
{
    String release =
    item["release_date"].as<String>();


    if(!isFutureDate(release))
        continue;


    if(index >= 3)
        break;


    movies[index].title = item["title"].as<String>();


    movies[index].date =
    formatDate(release);


    index++;
}
    }


    http.end();
}