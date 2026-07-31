#ifndef TMDB_H
#define TMDB_H

#include <Arduino.h>

struct Movie
{
    String title;
    String date;
};

extern Movie movies[3];

void fetchMovies();

#endif