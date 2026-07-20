#include "config.h"
#include <time.h>
#include <stdbool.h>
#include <curses.h>

#ifndef HABIT_H
#define HABIT_H

typedef struct {
    char name[name_max_length];
    time_t last_done;
    int year;
    bool history[days_in_year];
} Habit;

int get_streak(Habit habit, int today);

void mark_habit_done(Habit *habit, int yday);

void add_habit(Habit *list, int *current_total);

void delete_habit(int index, Habit *habits, int *current_total);

void add_habit_logic(Habit *list, int *current_total, WINDOW *win);

void rename_habit_logic(Habit *habit, WINDOW *win);

#endif
