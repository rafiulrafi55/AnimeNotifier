#ifndef ANIME_H
#define ANIME_H

#include <Arduino.h>

struct Anime
{
    String title;
    String time;
};

extern Anime animeList[3];

#endif