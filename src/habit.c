#include "habit.h"
#include <string.h>

int get_streak(Habit habit, int today)
{
    if(!habit.history[today])
        return 0; // No streak
    int day = today;

    while(day >= 0) {
        if(habit.history[day])
            day--;
        else break;
    }
    return today - day;
}

void mark_habit_done(Habit *habit, int yday) {
    habit->history[yday] = !habit->history[yday];
    if(habit->history[yday]) 
        habit->last_done = time(NULL);
    else
        habit->last_done = 0;
}

static bool get_text_input(WINDOW *win, char *buffer, int max_len) {
    int char_count = strlen(buffer);
    int ch;
    curs_set(1);
    
    // If editing existing text, print it first
    mvwprintw(win, 1, 1, "%s", buffer); 
    wrefresh(win);

    while(1) {
        ch = wgetch(win);
        if(ch == key_escape) {
            curs_set(0);
            return false; // User cancelled
        }
        else if(ch == key_enter) {
            break; // User finished
        }
        else if(ch == KEY_BACKSPACE || ch == 127) { // Handle 127 for Mac/some terms
            if(char_count > 0) {
                char_count--;
                buffer[char_count] = '\0';
                mvwaddch(win, 1, 1 + char_count, ' '); // Clear visual
                wmove(win, 1, 1 + char_count);         // Move cursor back
            }
        }
        else if(ch >= 32 && ch <= 126 && char_count < max_len - 1) {
            buffer[char_count] = (char)ch;
            char_count++;
            buffer[char_count] = '\0';
            waddch(win, ch);
        }
        wrefresh(win);
    }
    curs_set(0);
    return true;
}

void add_habit_logic(Habit *list, int *current_total, WINDOW *win) {
    char temp_name[name_max_length] = {0};

    do {
        if(!get_text_input(win, temp_name, name_max_length)) {
            delwin(0);
            return;
        }
    } while(strlen(temp_name) <= 0);
    strncpy(list[*current_total].name,
            temp_name, name_max_length - 1);
    list[*current_total].name[name_max_length - 1] = '\0';

    time_t now = time(NULL);

    list[*current_total].last_done = 0;
    list[*current_total].year = localtime(&now)->tm_year + 1900;
    for(int i = 0; i < days_in_year; i++)
        list[*current_total].history[i] = false;
    (*current_total)++;
    
}

void delete_habit(int index, Habit *habits, int *current_total)
{
    int i;
    for(i = index; i < (*current_total) - 1; i++) {
        habits[i] = habits[i+1];
    }
    (*current_total)--;
}

void rename_habit_logic(Habit *habit, WINDOW *win)
{
    char temp_name[name_max_length] = {0};
    strncpy(temp_name, habit->name, name_max_length - 1);
    temp_name[name_max_length - 1] = '\0';
    do {
        if(!get_text_input(win, temp_name, name_max_length)) {
            delwin(0);
            return;
        }
    } while(strlen(temp_name) <= 0);
    strncpy(habit->name, temp_name, name_max_length - 1);
    habit->name[name_max_length - 1] = '\0';
}

