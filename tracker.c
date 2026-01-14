#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
    idx_delete,
    idx_exit,
    menu_count
};

#define CMD_ADD -1
#define CMD_SETTINGS -2
#define CMD_DELETE -3
#define CMD_EXIT -4

// --- 2. Type Definitions ---
typedef void (*draw_item_func)(int index, int y, int x, bool highlighted, void *arg);

typedef struct Habit {
    char name[name_max_length];
    int completions;
    int completed;
} Habit;

static int menu_engine(int count, int start_y, int start_x, int *initial_highlight, bool horizontal_list,
        bool interactive, draw_item_func draw_item, void *arg)
{
    int key;
    int highlight = interactive ? *initial_highlight : -1;
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
            case '1': 
                return CMD_ADD;
            case '2': 
                return CMD_SETTINGS;
            case '3': 
                *initial_highlight = highlight;
                return CMD_DELETE;
            case '4': 
                return CMD_EXIT;
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
        "(3) Delete",
        "(4) Exit"
    };

    menu_engine(menu_count, start_y, start_x, NULL, true, false, draw_bar_item, menu_items);
}

static void add_new_habit(Habit *list, int *current_total) {
    if(*current_total >= max_habits_amount) return;

    curs_set(1);
    echo();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int height = 3;
    int width = 50;
    int start_y = (rows - height) / 2;
    int start_x = (cols - width) / 2;

    WINDOW *win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    
    wbkgd(win, COLOR_PAIR(3));
    box(win, 0, 0); 

    const char *prompt = " Your habit: ";
    mvwprintw(win, 1, 1, "%s", prompt);
    wmove(win, 1, 1 + strlen(prompt));
    wrefresh(win);

    wgetnstr(win, list[*current_total].name, name_max_length - 1);
    timeout(default_timeout);
    list[*current_total].completions = 0;
    list[*current_total].completed = 0;
    (*current_total)++;

    noecho();
    curs_set(0);
    delwin(win);
}

static void delete_habit(int index, Habit *habits, int *current_total)
{
    int i;
    for(i = index; i < *current_total; i++) {
        habits[i] = habits[i+1];
    }
    (*current_total)--;
}

// Forward declaration so main_screen and analyze_options can talk to each other
static void analyze_options(int option, int highlight, Habit *habits, int *current_total, int x, int y);

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

        if (*habits_total == 0)
            mvprintw(start_y, start_x, "No habits yet. See menu bar for possible actions.");

        int choice = menu_engine(*habits_total, start_y, start_x, &current_highlight, false, true, draw_habit_item, habits);

        if(choice < 0) {
            analyze_options(choice, current_highlight, habits, habits_total, start_x, start_y);
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

static bool confirm_delete(const char *habit_name) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int height = 8;
    int width = 50;
    int start_y = (rows - height) / 2;
    int start_x = (cols - width) / 2;

    WINDOW *win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    
    wbkgd(win, COLOR_PAIR(3));
    box(win, 0, 0); 

    // 4. Draw Content
    wattron(win, A_BOLD);
    mvwprintw(win, 1, (width - 14) / 2, " CONFIRMATION "); // 14 here is the length of the string passed
    wattroff(win, A_BOLD);

    mvwprintw(win, 3, (width - 32) / 2, "Are you sure you want to delete:"); // 32 is the length of the string
    
    // Highlight the habit name in Red to show it's the target of deletion
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, 4, (width - strlen(habit_name) - 2) / 2, "'%s'?", habit_name);
    wattroff(win, COLOR_PAIR(4));

    // Drawing "Buttons"
    int btn_y = 6;
    
    // The "Cancel" option (Neutral/Minimalist)
    mvwaddstr(win, btn_y, (width / 2) + 2, "[N]o");

    // The "Delete" option (Highlighted in Red)
    wattron(win, COLOR_PAIR(4));
    mvwaddstr(win, btn_y, (width / 2) - 12, "[Y]es");
    wattroff(win, COLOR_PAIR(4));

    wrefresh(win);

    // 5. Input Loop
    bool result = false;
    while (1) {
        int ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') {
            result = true;
            break;
        } else if (ch == 'n' || ch == 'N' || ch == key_escape) {
            result = false;
            break;
        }
    }

    delwin(win);
    return result;
}

static void analyze_options(int option, int highlight, Habit *habits, int *current_total, int x, int y) {
    switch(option) {
        case CMD_ADD:
            add_new_habit(habits, current_total);
            break;
        case CMD_SETTINGS:
            // Placeholder for settings
            break;
        case CMD_DELETE:
            if(confirm_delete(habits[highlight].name))
                delete_habit(highlight, habits, current_total);
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
    init_pair(1, COLOR_CYAN, COLOR_BLACK); // Active Habit
    init_pair(2, COLOR_BLACK, COLOR_WHITE); // Menu Bar
    init_pair(3, COLOR_BLACK, COLOR_WHITE);
    init_pair(4, COLOR_RED, COLOR_WHITE);
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
