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
    habit_fields = 3,
    bar_gap = 4
};

enum menu_indices {
    idx_add = 0,
    idx_settings,
    idx_exit,
    menu_count
};

#define CMD_ADD -1
#define CMD_SETTINGS -2
#define CMD_EXIT -3

// --- 2. Type Definitions ---
typedef void (*draw_item_func)(int index, int y, int x, bool highlighted, void *arg);

typedef struct Habit {
    char name[name_max_length];
    int completions;
    int completed;
} Habit;

static int menu_engine(int count, int start_y, int start_x, int initial_highlight, bool horizontal_list,
        bool interactive, draw_item_func draw_item, void *arg)
{
    int key;
    int highlight = initial_highlight;
    int cur_y, cur_x;
    
    // Disable timeout so the menu waits forever for user input
    timeout(-1); 

    while(1) {
        int x_offset = start_x;
        for(int i = 0; i < count; i++) {
            if(i == highlight && interactive) attron(COLOR_PAIR(1));
            else if(!interactive) attron(COLOR_PAIR(2));
            
            if(!horizontal_list)
                draw_item(i, start_y + i, start_x, i == highlight, arg);
            else {
                draw_item(i, start_y, x_offset, i == highlight, arg);
                getyx(stdscr, cur_y, cur_x);
                x_offset = cur_x + bar_gap;
            }

            attroff(COLOR_PAIR(1));
            attroff(COLOR_PAIR(2));
        }
        refresh();
        
        if(!interactive) return highlight;

        key = getch();
        switch(key) {
            case '1': return CMD_ADD;
            case '2': return CMD_SETTINGS;
            case '3': return CMD_EXIT;
            case KEY_UP:
                highlight = (highlight - 1 + count) % count;
                break;
            case KEY_DOWN:
                highlight = (highlight + 1) % count;
                break;
            case key_enter:
                return highlight; 
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
    mvprintw(y, x, "[%c] %-15s: %2d completions.", 
             habits[i].completed ? 'x' : ' ',
             habits[i].name, habits[i].completions);
}

void draw_bar_item(int i, int y, int x, bool highlighted, void *arg)
{
    const char **items = (const char **)arg;
    mvaddstr(y, x, items[i]);
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

static void menu_bar(int start_x, int start_y)
{
    static const char *menu_items[menu_count] = {
        "(1) Add new habit",
        "(2) Settings",
        "(3) Exit"
    };

    menu_engine(menu_count, start_y, start_x, -1, true, false, draw_bar_item, menu_items);
}

static void add_new_habit(Habit *list, int *current_total, int x, int y) {
    if(*current_total >= max_habits_amount) return;
    clear();
    curs_set(1);
    echo();
    mvprintw(y, x, "New habit name: ");
    getnstr(list[*current_total].name, name_max_length - 1);
    timeout(default_timeout);
    list[*current_total].completions = 0;
    list[*current_total].completed = 0;
    noecho();
    curs_set(0);
    (*current_total)++;
}

// Forward declaration so main_screen and analyze_options can talk to each other
static void analyze_options(int option, Habit *habits, int *current_total, int x, int y);

static void main_screen(Habit *habits, int *habits_total, int start_y, int start_x) {
    int current_highlight = 0;
    int row, col;

    while(1) {
        getmaxyx(stdscr, row, col);
        clear();

        attron(A_BOLD | A_UNDERLINE);
        mvprintw(start_y - 3, start_x, "HABITS LIST");
        attroff(A_BOLD | A_UNDERLINE);

        menu_bar(start_x, row - 2); 

        if (*habits_total == 0) {
            mvprintw(start_y, start_x, "No habits yet. Press ESC for menu.");
            int cmd = getch();
            if (cmd == '1') analyze_options(idx_add, habits, habits_total, start_x, start_y);
            if (cmd == '3') analyze_options(idx_exit, habits, habits_total, start_x, start_y);
        } else {
            int choice = menu_engine(*habits_total, start_y, start_x, current_highlight, false, true, draw_habit_item, habits);

            if(choice < 0) {
                analyze_options(choice, habits, habits_total, start_x, start_y);
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
}

static void analyze_options(int option, Habit *habits, int *current_total, int x, int y) {
    switch(option) {
        case CMD_ADD:
            add_new_habit(habits, current_total, x, y);
            break;
        case CMD_SETTINGS:
            // Placeholder for settings
            break;
        case CMD_EXIT:
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
    int row, col, habits_total = 0;
    Habit my_habits[max_habits_amount];

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_GREEN); // Active Highlight
    init_pair(2, COLOR_BLACK, COLOR_CYAN); // Menu Bar
    curs_set(0);

    getmaxyx(stdscr, row, col);
    int start_x = (col / 2) - 20;

    load_habits(my_habits, &habits_total);

    // Enter the main app loop
    main_screen(my_habits, &habits_total, row/2, start_x);

    // Cleanup (though exit(0) in analyze_options usually handles this)
    endwin();
    return 0;
}
