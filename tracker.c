#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

const char new_habit_message[] = "Add new habit";
enum { key_escape = 27, key_enter = 10,
       default_timeout = 100, name_max_length = 50,
       max_habits_amount = 5};

enum menu_choices {
    menu_cancel = -1,
    menu_add = 0,
    menu_settings,
    menu_exit,
    menu_count
};

typedef struct Habit {
    char name[name_max_length];
    int count;
} Habit;

static void colors_init()
{
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
}

static int start_menu(int start_x, int start_y)
{
    static const char *menu_items[menu_count] = {
        "Add new habit",
        "Settings",
        "Exit"
    };
    int key;
    int highlight = 0; // Index of the currently selected item
    timeout(-1); 

    while(1) {
        // 1. Draw all items
        for(int i = 0; i < menu_count; i++) {
            if(i == highlight) {
                attron(COLOR_PAIR(1) | A_REVERSE); // Highlighted style
            }

            mvaddstr(start_y + i, start_x, menu_items[i]);

            if(i == highlight) {
                attroff(COLOR_PAIR(1) | A_REVERSE);
            }
        }
        refresh();

        key = getch();
        switch(key) {
            case KEY_UP:
                highlight--;
                if(highlight < 0) highlight = menu_count - 1;
                break;
            case KEY_DOWN:
                highlight++;
                if(highlight >= menu_count) highlight = 0;
                break;
            case key_enter:
                return highlight;
            case key_escape:
                return -1;
        }
    }
    timeout(default_timeout);
}

static void add_new_habbit(Habit *list, int *current_total)
{
    if(*current_total >= max_habits_amount)
        return;

    Habit new_h;
    snprintf(new_h.name, 50, "Read Book");
    new_h.count = 0;

    list[*current_total] = new_h;
    (*current_total)++;

    /* Saving to the disk will be implemented below */
}

static void analyze_options(int option)
{
    switch(option) {
        case menu_add:
            break;
        case menu_settings:
            break;
        case menu_exit:
            endwin();
            exit(0);
            break;
        case menu_cancel:
            break;
    }
}

int main() {
    int row, col, key;
    int menu_option;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    timeout(default_timeout);
    getmaxyx(stdscr, row, col);
    curs_set(0);
    colors_init();

    Habit my_habits[max_habits_amount];

    menu_option = start_menu(col/2, row/2);
    analyze_options(menu_option);
    for(;;) {
        key = getch();
        switch(key) {
        case key_escape:
            menu_option = start_menu(col/2, row/2);
            analyze_options(menu_option);
            break;
        }
    }
    endwin();
    return 0;
}
