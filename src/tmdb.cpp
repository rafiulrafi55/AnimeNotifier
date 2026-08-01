#include "tmdb.h"
#include "secrets.h"
#include "time_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

Movie movies[MAX_MOVIES];
int movieCount = 0;
MovieChoice seasonMovieChoices[MAX_MOVIE_CHOICES];
int seasonMovieCount = 0;
String selectedMovieTitles[MAX_MOVIE_CHOICES];
int selectedMovieTitleCount = 0;

static int currentYearValue()
{
    time_t nowRaw = time(nullptr);
    struct tm nowTime;
    localtime_r(&nowRaw, &nowTime);
    return nowTime.tm_year + 1900;
}

static bool isSelectedMovieTitle(const String &title)
{
    for(int i = 0; i < selectedMovieTitleCount; i++)
    {
        if(selectedMovieTitles[i] == title)
            return true;
    }

    return false;
}

bool isFutureDate(String date)
{
    if(date.length() < 10)
        return false;

    int year = date.substring(0,4).toInt();
    int month = date.substring(5,7).toInt();
    int day = date.substring(8,10).toInt();

    String today = currentDate();

    int currentYear = today.substring(0,4).toInt();
    int currentMonth = today.substring(5,7).toInt();
    int currentDay = today.substring(8,10).toInt();

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

bool fetchMovies()
{
    if(WiFi.status() != WL_CONNECTED)
        return false;

    String today = currentDate();
    String startDate = today.length() >= 10 ? today : "1970-01-01";

    String url =
    "https://api.themoviedb.org/3/discover/movie?api_key="
    + String(TMDB_API_KEY)
    + "&language=en-US&page=1"
    + "&primary_release_date.gte=" + startDate
    + "&sort_by=popularity.desc";

    HTTPClient http;
    http.begin(url);

    int code = http.GET();
    if(code != 200)
    {
        http.end();
        return false;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(12000);

    if(deserializeJson(doc, payload))
    {
        http.end();
        return false;
    }

    JsonArray results = doc["results"];

    struct MovieCandidate
    {
        String title;
        String date;
        int popularity;
        bool selected;
    };

    MovieCandidate candidates[20];
    int candidateCount = 0;

    for(JsonObject item : results)
    {
        if(candidateCount >= 20)
            break;

        String release = item["release_date"].as<String>();

        if(!isFutureDate(release))
            continue;

        candidates[candidateCount].title = item["title"].as<String>();
        candidates[candidateCount].date = formatDate(release);
        candidates[candidateCount].popularity = item["popularity"].as<int>();
        candidates[candidateCount].selected = isSelectedMovieTitle(candidates[candidateCount].title);
        candidateCount++;
    }

    for(int i = 0; i < candidateCount - 1; i++)
    {
        for(int j = i + 1; j < candidateCount; j++)
        {
            if(candidates[j].popularity > candidates[i].popularity)
            {
                MovieCandidate temp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = temp;
            }
        }
    }

    movieCount = 0;

    for(int i = 0; i < MAX_MOVIES; i++)
    {
        movies[i].title = "";
        movies[i].date = "";
        movies[i].popularity = 0;
    }

    int outputIndex = 0;

    if(selectedMovieTitleCount > 0)
    {
        for(int i = 0; i < candidateCount; i++)
        {
            if(!candidates[i].selected)
                continue;

            movies[outputIndex].title = candidates[i].title;
            movies[outputIndex].date = candidates[i].date;
            movies[outputIndex].popularity = candidates[i].popularity;
            movieCount++;
            outputIndex++;
            break;
        }
    }

    for(int i = 0; i < candidateCount && outputIndex < MAX_MOVIES; i++)
    {
        if(outputIndex == 0)
        {
            movies[outputIndex].title = candidates[i].title;
            movies[outputIndex].date = candidates[i].date;
            movies[outputIndex].popularity = candidates[i].popularity;
            movieCount++;
            outputIndex++;
            continue;
        }

        if(movies[0].title.length() > 0 && candidates[i].title == movies[0].title)
            continue;

        movies[outputIndex].title = candidates[i].title;
        movies[outputIndex].date = candidates[i].date;
        movies[outputIndex].popularity = candidates[i].popularity;
        movieCount++;
        outputIndex++;
    }

    http.end();
    return true;
}

void fetchSeasonMovieChoices()
{
    if(WiFi.status() != WL_CONNECTED)
        return;

    seasonMovieCount = 0;
    int currentYear = currentYearValue();
    String today = currentDate();
    String startDate = today.length() >= 10 ? today : (String(currentYear) + "-01-01");
    String endDate = String(currentYear) + "-12-31";

    const int maxPerMonth = 5;
    MovieChoice monthTop[12][maxPerMonth];
    int monthCount[12] = {0};

    for(int page = 1; page <= 12; page++)
    {
        String url =
        "https://api.themoviedb.org/3/discover/movie?api_key="
        + String(TMDB_API_KEY)
        + "&language=en-US&page=" + String(page)
        + "&primary_release_date.gte=" + startDate
        + "&primary_release_date.lte=" + endDate
        + "&sort_by=popularity.desc";

        HTTPClient http;
        http.begin(url);

        int code = http.GET();
        if(code != 200)
        {
            http.end();
            break;
        }

        String payload = http.getString();
        JsonDocument doc;
        if(deserializeJson(doc, payload))
        {
            http.end();
            break;
        }

        JsonArray results = doc["results"];
        if(results.size() == 0)
        {
            http.end();
            break;
        }

        for(JsonObject item : results)
        {
            String release = item["release_date"].as<String>();

            if(!isFutureDate(release))
                continue;

            int releaseYear = release.substring(0, 4).toInt();
            if(releaseYear != currentYear)
                continue;

            int month = release.substring(5, 7).toInt();
            if(month < 1 || month > 12)
                continue;

            int monthIndex = month - 1;
            if(monthCount[monthIndex] >= maxPerMonth)
                continue;

            String title = item["title"].as<String>();
            int movieId = item["id"].as<int>();
            int popularity = item["popularity"].as<int>();
            String prettyDate = formatDate(release);

            bool duplicateInMonth = false;
            for(int i = 0; i < monthCount[monthIndex]; i++)
            {
                if(monthTop[monthIndex][i].title == title)
                {
                    duplicateInMonth = true;
                    break;
                }
            }

            if(duplicateInMonth)
                continue;

            int writeIndex = monthCount[monthIndex];
            monthTop[monthIndex][writeIndex].id = movieId;
            monthTop[monthIndex][writeIndex].title = title;
            monthTop[monthIndex][writeIndex].date = prettyDate;
            monthTop[monthIndex][writeIndex].popularity = popularity;
            monthTop[monthIndex][writeIndex].selected = isSelectedMovieTitle(title);
            monthCount[monthIndex]++;
        }

        http.end();
    }

    for(int month = 0; month < 12 && seasonMovieCount < MAX_MOVIE_CHOICES; month++)
    {
        for(int i = 0; i < monthCount[month] && seasonMovieCount < MAX_MOVIE_CHOICES; i++)
        {
            seasonMovieChoices[seasonMovieCount++] = monthTop[month][i];
        }
    }
}
