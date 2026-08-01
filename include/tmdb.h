#ifndef TMDB_H
#define TMDB_H

#include <Arduino.h>

struct Movie
{
    String title;
    String date;
    int popularity;
};

struct MovieChoice
{
    int id;
    String title;
    String date;
    int popularity;
    bool selected;
};

const int MAX_MOVIES = 10;
const int MAX_MOVIE_CHOICES = 200;

extern Movie movies[MAX_MOVIES];
extern int movieCount;
extern MovieChoice seasonMovieChoices[MAX_MOVIE_CHOICES];
extern int seasonMovieCount;
extern String selectedMovieTitles[MAX_MOVIE_CHOICES];
extern int selectedMovieTitleCount;

bool fetchMovies();
void fetchSeasonMovieChoices();

#endif