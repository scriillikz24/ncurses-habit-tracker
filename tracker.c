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
    days_in_week = 7,
    weeks_in_year = 53
};

enum menu_indices {
    idx_add = 0,
    idx_settings,
    idx_delete,
    idx_year_grid,
    idx_exit,
    menu_count
};

#define CMD_ADD -1
#define CMD_SETTINGS -2
#define CMD_DELETE -3
#define CMD_YEAR_GRID - 4
#define CMD_EXIT -5

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

    for(int i = 0; i < total; i++) {
        if(list[i].last_done == 0) continue;

        struct tm *last_tm = localtime(&list[i].last_done);

        if(cur_day != last_tm->tm_yday) 
            list[i].history[cur_day] = false;
    }
}

static void mark_habit_done(Habit *habit, int yday) {
    habit->history[yday] = !habit->history[yday];
    if(habit->history[yday]) {
        habit->completions++;
        habit->last_done = time(NULL);
    }
    else {
        habit->completions--;
        habit->last_done = 0;
    }
}

char get_history_char(Habit h, int today, int offset)
{
    int index = today - offset;
    if(index < 0)
        index += 366;
    return h.history[index] ? 'x' : ' ';
}

static void draw_habit_item(int i, int y, int x, int selected_yday, bool highlighted, Habit *habits) {
    time_t now = time(NULL);
    int real_today = localtime(&now)->tm_yday;

    int day_offset = real_today - selected_yday;
    int target_column = days_in_week - 1 - day_offset;

    mvprintw(y, x, "%-30.30s ", habits[i].name);
    for(int wd = 0; wd < days_in_week; wd++) {
        int history_idx = real_today - (days_in_week - 1 - wd);
        if(history_idx < 0) 
            history_idx += days_in_year;
        char c = habits[i].history[history_idx] ? 'x' : ' ';

        if(wd == target_column && highlighted) attron(COLOR_PAIR(2));
        printw("[%c]", c);
        if(wd == target_column && highlighted) attroff(COLOR_PAIR(2));
    }
}

static void upload_to_disk(Habit *habits, int current_total) {
    FILE *dest = fopen(HABITS_FILE, "w");
    if(!dest) return;

    for(int i = 0; i < current_total; i++) {
        char s[days_in_year + 1];
        //memset(s, '0', days_in_year);
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

static void action_bar(int rows, int cols)
{
    static const char *menu_items[menu_count] = {
        "(1) Add habit",
        "(2) Settings",
        "(3) Delete",
        "(4) Year Grid",
        "(5) Exit"
    };
    int total_width = 0;
    for(int i = 0; i < 5; i++) total_width += strlen(menu_items[i]) + bar_gap;

    // Center the bar at the bottom of the screen
    int x_offset = (cols - total_width) / 2;
    int y_pos = rows - 2;

    attron(COLOR_PAIR(2));
    // Draw a background strip for the menu
    mvhline(y_pos, 0, ' ', cols); 
    
    for(int i = 0; i < 5; i++) {
        mvaddstr(y_pos, x_offset, menu_items[i]);
        x_offset += strlen(menu_items[i]) + bar_gap;
    }
    attroff(COLOR_PAIR(2));
}

static void add_new_habit(Habit *list, int *current_total) {
    if(*current_total >= max_habits_amount) return;

    clear();
    refresh();
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
    for(i = index; i < (*current_total) - 1; i++) {
        habits[i] = habits[i+1];
    }
    (*current_total)--;
}

static void print_week_labels(int y, int x)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int today_wday = t->tm_wday;

    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

    move(y, x + 31);
    for(int i = 0; i < days_in_week; i++) {
        int idx = (today_wday - (days_in_week - 1 - i) + days_in_week) % days_in_week;
        printw("%s ", days[idx]);
    }
}

static bool confirm_delete(const char *habit_name) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    clear();
    refresh();

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

static void year_grid(Habit *habit) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    mvprintw(2, (cols - 20)/2, "Yearly: %s", habit->name);
    
    int start_y = (rows - 15) / 2;
    int start_x = (cols - 80) / 2;

    for (int i = 0; i < 365; i++) {
        int week = i / 7;
        int day = i % 7;
        int color = habit->history[i] ? COLOR_PAIR(6) : COLOR_PAIR(5);
        
        attron(color);
        mvaddstr(start_y + day, start_x + (week * 3), "  ");
        attroff(color);
    }
    refresh();
    getch();
}

static void main_screen(Habit *habits, int *total) {
    int highlight = 0;
    time_t now = time(NULL);
    int real_today = localtime(&now)->tm_yday;
    int view_day = real_today;

    while(1) {
        int r, c;
        getmaxyx(stdscr, r, c);
        
        // Logical centering
        int list_x = (c / 2) - 25;
        int list_y = (r / 2) - (*total / 2);

        // 1. Use erase() instead of clear() to stop flickering
        erase();

        // 2. Draw Header
        attron(A_BOLD | A_UNDERLINE);
        mvprintw(list_y - 4, list_x, "HABITS TRACKER");
        attroff(A_BOLD | A_UNDERLINE);

        // 3. Draw Column Labels
        print_week_labels(list_y - 1, list_x);

        // 4. Draw List
        if(*total == 0) {
            mvprintw(list_y, list_x, "No habits found. Press (1) to add.");
        } else {
            for(int i = 0; i < *total; i++) {
                if(i == highlight) attron(COLOR_PAIR(1));
                draw_habit_item(i, list_y + i, list_x, view_day, i == highlight, habits);
                attroff(COLOR_PAIR(1));
            }
        }

        // 5. Draw Action Bar (passing screen dimensions for centering)
        action_bar(r, c);

        // 6. SINGLE refresh at the end
        refresh();

        int ch = getch();
        switch(ch) {
            case KEY_UP:   
                if(*total > 0) highlight = (highlight - 1 + *total) % *total; 
                break;
            case KEY_DOWN: 
                if(*total > 0) highlight = (highlight + 1) % *total; 
                break;
            case KEY_LEFT: 
                if(view_day > real_today - 6) view_day--; 
                break;
            case KEY_RIGHT: 
                if(view_day < real_today) view_day++; 
                break;
            case '1': 
                add_new_habit(habits, total); 
                break;
            case '3': 
                if(*total > 0 && confirm_delete(habits[highlight].name)) {
                    delete_habit(highlight, habits, total);
                    if(highlight >= *total && highlight > 0) highlight--;
                }
                break;
            case '4': 
                if(*total > 0) year_grid(&habits[highlight]); 
                break;
            case key_enter: 
            case 13: 
                if(*total > 0) mark_habit_done(&habits[highlight], view_day); 
                break;
            case '5': 
                upload_to_disk(habits, *total);
                endwin(); 
                exit(0);
        }
    }
}

static void year_grid_two(Habit *habit) {
    clear();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    // Approximate start days for months (non-leap year)
    int m_starts[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    int weeks_per_row = 26;
    int grid_width = (weeks_per_row * 3) + 7;
    int grid_height = 20; // Expanded to fit month labels

    int start_y = (rows - grid_height) / 2;
    int start_x = (cols - grid_width) / 2;

    mvprintw(start_y - 3, start_x, "Yearly Progress: %s", habit->name);

    // 1. Draw Month Labels
    for (int m = 0; m < 12; m++) {
        int week = m_starts[m] / 7;
        int x_pos, y_pos;

        if (week < weeks_per_row) {
            x_pos = start_x + 6 + (week * 3);
            y_pos = start_y - 1; // Above top grid
        } else {
            x_pos = start_x + 6 + ((week - weeks_per_row) * 3);
            y_pos = start_y + 8; // Above bottom grid
        }
        mvaddstr(y_pos, x_pos, months[m]);
    }

    // 2. Draw Grid and Day Labels
    for (int i = 0; i < 365; i++) {
        int week = i / 7;
        int day = i % 7;
        int x, y;

        if (week < weeks_per_row) {
            x = start_x + 6 + (week * 3);
            y = start_y + day;
            if (week == 0) mvaddstr(y, start_x + 1, days[day]);
        } else {
            x = start_x + 6 + ((week - weeks_per_row) * 3);
            y = start_y + day + 9; // Stacked below
            if (week == weeks_per_row) mvaddstr(y, start_x + 1, days[day]);
        }

        int color = habit->history[i] ? COLOR_PAIR(6) : COLOR_PAIR(5);
        attron(color);
        mvaddstr(y, x, "  ");
        attroff(color);
    }

    mvprintw(start_y + 18, start_x, "Press any key to return...");
    refresh();
    timeout(-1);
    getch();
    timeout(default_timeout);
}

static void load_habits(Habit *habits, int *current_total) {
    FILE *from = fopen(HABITS_FILE, "r");
    if(!from) return;
    char line[512];
    int i = 0;
    while(fgets(line, sizeof(line), from) && i < max_habits_amount) {
        char s[days_in_year + 1];
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
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK); 
    init_pair(2, COLOR_BLACK, COLOR_WHITE); 
    init_pair(3, COLOR_WHITE, COLOR_BLACK); // Highlighted cell
    init_pair(4, COLOR_RED, COLOR_WHITE);
    init_pair(5, COLOR_WHITE, 235); 
    init_pair(6, COLOR_WHITE, COLOR_GREEN);
    curs_set(0);

    int total = 0;
    Habit my_habits[max_habits_amount];

    load_habits(my_habits, &total);
    check_and_reset_habits(my_habits, total);
    main_screen(my_habits, &total);

    endwin();
    return 0;
}
