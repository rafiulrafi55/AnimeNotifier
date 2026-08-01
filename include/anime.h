#ifndef ANIME_H
#define ANIME_H

#include <Arduino.h>

const int MAX_ANIME_CHOICES = 1000;

struct Anime
{
    String title;
    String time;
    long airingAt;
    String description;
};

struct AnimeChoice
{
    int id;
    String title;
    bool selected;
};

extern Anime animeList[3];
extern AnimeChoice seasonAnimeChoices[MAX_ANIME_CHOICES];
extern int seasonAnimeCount;
extern String selectedAnimeTitles[MAX_ANIME_CHOICES];
extern int selectedAnimeTitleCount;

#endif