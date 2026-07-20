#include "habit.h"

#ifndef UI_H
#define UI_H

void draw_habit_item(int y, int x, int selected_yday, bool highlighted, Habit habit);

void add_habit(Habit *list, int *current_total);

void rename_habit(Habit *habit);

void print_week_labels(int y, int x);

bool confirm_delete(const char *habit_name);

void color_themes_menu(int sd_bar_len);

void draw_calendar(Habit *h);

void draw_status_bar(int rows, int cols, int start_x, Habit *habits, int total, int view_day);

void draw_side_bar(int rows, int len, int is_called);

void main_screen(Habit *habits, int *total);

#endif
