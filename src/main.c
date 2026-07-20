#include "habit.h"
#include "colors.h"
#include "save.h"
#include "ui.h"

#include <curses.h>

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    init_colors();
    curs_set(0);
    timeout(-1);

    int total = 0;
    Habit my_habits[max_habits_amount];

    load_habits(my_habits, &total);
    main_screen(my_habits, &total);

    endwin();
    return 0;
}
