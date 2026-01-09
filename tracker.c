#include <curses.h>
#include <stdio.h>
#include <stdlib.h>

#define HABITS_FILE ".habits.csv"

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

static void add_new_habit(Habit *list, int *current_total, int x, int y)
{
    if(*current_total >= max_habits_amount)
        return;

    clear();
    refresh();
    mvprintw(y, x, "Your habit: ");
    curs_set(1);
    echo();
    Habit new_h;
    getnstr(new_h.name, name_max_length - 1);
    new_h.count = 0;
    noecho();
    curs_set(0);
    clear();

    list[*current_total] = new_h;
    (*current_total)++;

    FILE *fp = fopen(HABITS_FILE, "a");
    if(!fp) {
        perror(HABITS_FILE);
        exit(1);
    }
    fprintf(fp, "%s,%d\n", new_h.name, new_h.count);
    fclose(fp);
}


static void analyze_options(int option, Habit *habits, int *current_total, int x, int y)
{
    switch(option) {
        case menu_add:
            add_new_habit(habits, current_total, x, y);
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

static void main_screen(Habit *habits, int habits_total, int row, int col)
{
    mvprintw(row/2, col/2, "HABITS LIST");
    int i;
    for(i = 0; i < habits_total; i++) {
        mvprintw(row/2 + 2 + i, col/2, "%s: %d completions.", habits[i].name, habits[i].count);
    }
}

static void load_habits(Habit *habits, int *current_total)
{
    FILE *from = fopen(HABITS_FILE, "r");
    if(!from) {
        return;
    }
    char line[128];
    int i = 0;
    while(fgets(line, sizeof(line), from)) {
        if(i >= max_habits_amount)
            break;
        int fields = sscanf(line, " %49[^,],%d", habits[i].name, &habits[i].count);
        if(fields == 2)
            i++;
    }
    *current_total = i;
    fclose(from);
}

int main() {
    int row, col, key;
    int menu_option, habits_total;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    timeout(default_timeout);
    getmaxyx(stdscr, row, col);
    curs_set(0);
    colors_init();

    habits_total = 0;
    Habit my_habits[max_habits_amount];
    load_habits(my_habits, &habits_total);

    main_screen(my_habits, habits_total, row, col);
    for(;;) {
        key = getch();
        switch(key) {
        case key_escape:
            clear();
            refresh();
            menu_option = start_menu(col/2, row/2);
            analyze_options(menu_option, my_habits, &habits_total, col/2, row/2);
            break;
        }
        main_screen(my_habits, habits_total, row, col);
    }
    endwin();
    return 0;
}
