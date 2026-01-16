#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define HABITS_FILE ".habits.csv"

// --- 1. Constants and Enums ---
enum { 
    key_escape = 27, 
    key_enter = 10,
    default_timeout = 100, 
    name_max_length = 50,
    max_habits_amount = 5, 
    habit_fields = 4,
    bar_gap = 4,
    days_in_year = 366,
    days_in_week = 7
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

typedef struct Habit {
    char name[name_max_length];
    int completions;
    time_t last_done;
    bool history[days_in_year];
} Habit;

static void check_and_reset_habits(Habit *list, int total)
{
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);

    int cur_day = current_time->tm_yday;
    int cur_year = current_time->tm_year;

    for(int i = 0; i < total; i++) {
        if(list[i].last_done == 0) continue;

        struct tm *last_tm = localtime(&list[i].last_done);

        if(cur_day != last_tm->tm_yday || cur_year != last_tm->tm_year) 
            list[i].history[cur_day] = false;
    }
}

static void mark_habit_done(Habit *habit, int day) {
    time_t now = time(NULL);

    habit->history[day] = !habit->history[day];
    if(habit->history[day]) {
        habit->completions++;
        habit->last_done = now;
    }
    else {
        habit->completions--;
        habit->last_done = 0;
    }
}

static void menu_bar_engine(int count, int start_y, int start_x, const char **menu_items)
{
    int cur_y, cur_x;
    for(int i = 0; i < count; i++) {
        int x_offset = start_x;
        for(int i = 0; i < count; i++) {
            attron(COLOR_PAIR(2));
            mvaddstr(start_y, x_offset, menu_items[i]);
            getyx(stdscr, cur_y, cur_x);
            x_offset = cur_x + bar_gap;
            attroff(COLOR_PAIR(2));
        }
        refresh();
    }
}

char get_history_char(Habit *h, int today, int offset)
{
    int index = today - offset;
    if(index < 0)
        index += 366;
    return h->history[index] ? 'x' : ' ';
}

void draw_habit_item(int i, int y, int x, int wday, bool highlighted, Habit *habits) { //wday - week day
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int ytoday = t->tm_yday;

    mvprintw(y, x, "%-30.30s ", habits[i].name);
    for(int d = 6; d >= 0; d--) {
        char c = get_history_char(&habits[i], ytoday, d);
        if(d == wday && highlighted) attron(COLOR_PAIR(3));
        printw("[%c]", c);
        if(d == wday && highlighted) attroff(COLOR_PAIR(3));
    }
}

static int main_screen_engine(int count, int start_y, int start_x, int *year_day, int wday, int *initial_highlight, Habit *habits) 
{
    int key;
    int highlight = *initial_highlight;

    timeout(-1); 

    while(1) {
        for(int i = 0; i < count; i++) {
            if(i == highlight) attron(COLOR_PAIR(1));
            draw_habit_item(i, start_y + i, start_x, wday, i == highlight, habits);
            attroff(COLOR_PAIR(1));
        }
        refresh();
        
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
            case KEY_LEFT:
                *year_day = (*year_day + 1) % days_in_year;
                wday = (wday + 1) % days_in_week;
                break;
            case KEY_RIGHT:
                *year_day = (*year_day - 1 + days_in_year) % days_in_year;
                wday = (wday - 1 + days_in_week) % days_in_week;
                break;
            case key_enter:
                return highlight; 
        }
    }
}

static void upload_to_disk(Habit *habits, int current_total) {
    FILE *dest = fopen(HABITS_FILE, "w");
    if(!dest) return;
    char s[days_in_year + 1];
    for(int i = 0; i < current_total; i++) {
        memset(s, '0', days_in_year);
        s[days_in_year] = '\0';
        for(int j = 0; j < days_in_year; j++)
            s[j] = habits[i].history[j] ? '1' : '0';
        s[days_in_year] = '\0';
        fprintf(dest, "%s,%d,%ld,%s\n", 
                habits[i].name, 
                habits[i].completions, 
                habits[i].last_done, 
                s);
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

    menu_bar_engine(menu_count, start_y, start_x, menu_items);
}

static void add_new_habit(Habit *list, int *current_total) {
    if(*current_total >= max_habits_amount) return;

    curs_set(1);
    echo();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int height = 3;
    int width = name_max_length + 10;
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
    list[*current_total].last_done = 0;
    for(int i = 0; i < days_in_year; i++)
        list[*current_total].history[i] = false;
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

static int get_week_day(int today, int offset)
{
    int index = today - offset;
    if(index < 0)
        index += 7;
    return index;
}

static void print_week_days(int start_y, int start_x)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int today = t->tm_wday;

    const char *week_days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

    move(start_y - 1, start_x + 31);
    for(int i = 0; i < days_in_week; i++) {
        int index = get_week_day(today, days_in_week - i - 1);
        printw("%s ", week_days[index]);
    }
}

static void analyze_options(int option, int highlight, Habit *habits, int *current_total, int x, int y);

static void main_screen(Habit *habits, int *habits_total, int start_y, int start_x) {
    int current_highlight = 0;
    int row, col;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int year_day = t->tm_yday;
    int week_day = t->tm_wday;

    while(1) {
        getmaxyx(stdscr, row, col);
        clear();

        attron(A_BOLD | A_UNDERLINE);
        mvprintw(start_y - 3, start_x, "HABITS LIST");
        attroff(A_BOLD | A_UNDERLINE);

        print_week_days(start_y, start_x);
        menu_bar(start_x, row - 2); 

        if (*habits_total == 0)
            mvprintw(start_y, start_x, "No habits yet. See menu bar for possible actions.");

        int choice = main_screen_engine(*habits_total, start_y, start_x, &year_day, week_day, &current_highlight, habits);

        if(choice < 0) {
            analyze_options(choice, current_highlight, habits, habits_total, start_x, start_y);
        } else {
            current_highlight = choice;
            mark_habit_done(&habits[choice], year_day);
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
    char s[days_in_year + 1];
    while(fgets(line, sizeof(line), from) && i < max_habits_amount) {
        if(sscanf(line, " %49[^,],%d,%ld,%s", habits[i].name, &habits[i].completions, 
                    &habits[i].last_done, s) == habit_fields) {
            for(int j = 0; j < days_in_year; j++)
                habits[i].history[j] = (s[j] == '1');
            i++;
        }
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
    check_and_reset_habits(my_habits, habits_total);

    // Enter the main app loop
    main_screen(my_habits, &habits_total, row/2, start_x);

    // Cleanup (though exit(0) in analyze_options usually handles this)
    endwin();
    return 0;
}
