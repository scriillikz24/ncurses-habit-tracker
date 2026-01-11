#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define HABITS_FILE ".habits.csv"

// --- 1. Constants and Enums ---
enum { 
    key_escape = 27, 
    key_enter = 10,
    default_timeout = 100, 
    name_max_length = 50,
    max_habits_amount = 5, 
    habit_fields = 3 
};

enum menu_choices {
    menu_cancel = -1,
    menu_add = 0,
    menu_settings,
    menu_exit,
    menu_count
};

// --- 2. Type Definitions ---
typedef void (*draw_item_func)(int index, int y, int x, bool highlighted, void *arg);

typedef struct Habit {
    char name[name_max_length];
    int completions;
    int completed;
} Habit;

static int menu_engine(int count, int start_y, int start_x, int initial_highlight, draw_item_func draw_item, void *arg) {
    int key;
    int highlight = initial_highlight;
    
    // Disable timeout so the menu waits forever for user input
    timeout(-1); 

    while(1) {
        for(int i = 0; i < count; i++) {
            if(i == highlight) attron(COLOR_PAIR(1) | A_REVERSE);
            
            // The engine calls the callback here to do the drawing
            draw_item(i, start_y + i, start_x, i == highlight, arg);

            if(i == highlight) attroff(COLOR_PAIR(1) | A_REVERSE);
        }
        refresh();

        key = getch();
        switch(key) {
            case KEY_UP:
                highlight = (highlight - 1 + count) % count;
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % count;
                break;
            case key_enter:
                timeout(default_timeout); // Restore timeout before leaving
                return highlight; 
            case key_escape:
                timeout(default_timeout); // Restore timeout before leaving
                return menu_cancel; 
        }
    }
}

// --- 4. Callbacks (Specific Drawing Logic) ---

void draw_start_menu_item(int i, int y, int x, bool highlighted, void *arg) {
    const char **items = (const char **)arg;
    mvaddstr(y, x, items[i]);
}

void draw_habit_item(int i, int y, int x, bool highlighted, void *arg) {
    Habit *habits = (Habit *)arg;
    mvprintw(y, x, "[%c] %s: %d completions.", 
             habits[i].completed ? 'x' : ' ',
             habits[i].name, habits[i].completions);
}

// --- 5. Logic and Screens ---

static void upload_to_disk(Habit *habits, int current_total) {
    FILE *dest = fopen(HABITS_FILE, "w");
    if(!dest) return;
    for(int i = 0; i < current_total; i++) {
        fprintf(dest, "%s,%d,%d\n", habits[i].name, habits[i].completions, habits[i].completed);
    }
    fclose(dest);
}

static void add_new_habit(Habit *list, int *current_total, int x, int y) {
    if(*current_total >= max_habits_amount) return;

    clear();
    mvprintw(y, x, "New habit name: ");
    curs_set(1);
    echo();
    getnstr(list[*current_total].name, name_max_length - 1);
    list[*current_total].completions = 0;
    list[*current_total].completed = 0;
    noecho();
    curs_set(0);
    (*current_total)++;
    clear();
}

static int start_menu(int start_x, int start_y) {
    static const char *menu_items[menu_count] = {
        "Add new habit",
        "Settings",
        "Exit"
    };
    clear();
    // We call the engine with the start_menu callback
    return menu_engine(menu_count, start_y, start_x, 0, draw_start_menu_item, menu_items);
}

// Forward declaration so main_screen and analyze_options can talk to each other
static void analyze_options(int option, Habit *habits, int *current_total, int x, int y);

static void main_screen(Habit *habits, int *habits_total, int start_y, int start_x) {
    int current_highlight = 0;

    while(1) {
        clear();
        attron(A_BOLD);
        mvprintw(start_y - 2, start_x, "HABITS LIST (ESC for Menu)");
        attroff(A_BOLD);

        if (*habits_total == 0) {
            mvprintw(start_y, start_x, "No habits yet. Press ESC for menu.");
        }

        int choice = menu_engine(*habits_total, start_y, start_x, current_highlight, draw_habit_item, habits);

        if(choice == menu_cancel) {
            int option = start_menu(start_x, start_y);
            analyze_options(option, habits, habits_total, start_x, start_y);
        } else {
            current_highlight = choice;
            habits[choice].completed = !habits[choice].completed;
            if(habits[choice].completed) 
                habits[choice].completions++;
            else
                habits[choice].completions--; // Optional: decrement if unchecked
        }
    }
}

static void analyze_options(int option, Habit *habits, int *current_total, int x, int y) {
    switch(option) {
        case menu_add:
            add_new_habit(habits, current_total, x, y);
            break;
        case menu_settings:
            // Placeholder for settings
            break;
        case menu_exit:
            upload_to_disk(habits, *current_total);
            endwin();
            exit(0);
        default: // Includes menu_cancel
            break;
    }
}

static void load_habits(Habit *habits, int *current_total) {
    FILE *from = fopen(HABITS_FILE, "r");
    if(!from) return;
    char line[128];
    int i = 0;
    while(fgets(line, sizeof(line), from) && i < max_habits_amount) {
        if(sscanf(line, " %49[^,],%d,%d", habits[i].name, &habits[i].completions, &habits[i].completed) == habit_fields)
            i++;
    }
    *current_total = i;
    fclose(from);
}

int main() {
    int row, col;
    int habits_total = 0;
    Habit my_habits[max_habits_amount];

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    getmaxyx(stdscr, row, col);
    curs_set(0);

    load_habits(my_habits, &habits_total);

    // Enter the main app loop
    main_screen(my_habits, &habits_total, row/2, col/2);

    // Cleanup (though exit(0) in analyze_options usually handles this)
    endwin();
    return 0;
}
