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
    name_max_length = 30,
    checkbox_offset = 30,
    dashboard_length = 49,
    max_habits_amount = 5, 
    habit_fields = 3,
    bar_gap = 4,
    days_in_year = 366,
    days_in_week = 7,
    weeks_in_year = 53,
    debug_day = 0
};

enum menu_indices {
    idx_add = 0,
    idx_delete,
    idx_rename,
    idx_calendar,
    idx_quit,
    menu_count
};

// --- 2. Type Definitions ---

typedef struct Habit {
    char name[name_max_length];
    time_t last_done;
    bool history[days_in_year];
} Habit;

static void mark_habit_done(Habit *habit, int yday) {
    habit->history[yday] = !habit->history[yday];
    if(habit->history[yday]) 
        habit->last_done = time(NULL);
    else
        habit->last_done = 0;
}

static int get_streak(Habit habit, int today)
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

static void draw_habit_item(int y, int x, int selected_yday, bool highlighted, Habit habit) {
    time_t now = time(NULL);
    int real_today = localtime(&now)->tm_yday + debug_day;

    int day_offset = real_today - selected_yday;
    int target_column = days_in_week - 1 - day_offset;

    int streak = get_streak(habit, real_today);

    // --- IMPROVED STREAK UI START ---
    move(y, x);

    if (streak == 0) {
        // State: Inactive (Dimmed Dash)
        attron(COLOR_PAIR(3)); 
        printw("  -  ");
        attroff(COLOR_PAIR(3)); 
    } 
    else if (streak < days_in_week) {
        // State: Spark (Yellow, standard weight)
        attron(COLOR_PAIR(6)); // Yellow
        printw(" %d ", streak);
        attroff(COLOR_PAIR(6));
    } 
    else {
        attron(COLOR_PAIR(5) | A_BOLD); // Red + Bold
        printw(" %d ", streak);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }
    // --- IMPROVED STREAK UI END ---

    int cur_y, cur_x;
    if(!highlighted) attron(COLOR_PAIR(3)); 
    printw("%s", habit.name);
    if(!highlighted) attroff(COLOR_PAIR(3)); 

    int checkbox_start_col = x + checkbox_offset;
    getyx(stdscr, cur_y, cur_x);
    
    // Fill remaining space with padding
    while(cur_x < checkbox_start_col) {
        addch(' ');
        cur_x++;
    }

    // Draw Checkboxes
    for(int wd = 0; wd < days_in_week; wd++) {
        int history_idx = real_today - (days_in_week - 1 - wd);
        if(history_idx < 0) history_idx += days_in_year;
        
        char c = habit.history[history_idx] ? 'x' : '.';
        
        int attr = (wd == target_column && highlighted) ? A_NORMAL : COLOR_PAIR(3);
        attron(attr);
        printw(" %c ", c);
        attroff(attr);
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

        fprintf(dest, "%s,%ld,%s\n", 
                habits[i].name, 
                habits[i].last_done, 
                s);
    }
    fclose(dest);
}

static void action_bar(int rows, int cols)
{
    static const char *menu_items[menu_count] = {
        "1 Add",
        "2 Delete",
        "3 Rename",
        "4 Calendar",
        "5 Quit"
    };
    int total_width = 0;
    for(int i = 0; i < menu_count; i++) 
        total_width += strlen(menu_items[i]) + bar_gap;

    // Center the bar at the bottom of the screen
    int x_offset = (cols - total_width) / 2;
    int y_pos = rows - 2;

    // Draw a background strip for the menu
    attron(COLOR_PAIR(3));
    mvhline(y_pos - 1, 0, ACS_HLINE, cols); 
    mvhline(y_pos + 1, 0, ACS_HLINE, cols); 
    attroff(COLOR_PAIR(3));
    
    for(int i = 0; i < menu_count; i++) {
        mvaddstr(y_pos, x_offset, menu_items[i]);
        x_offset += strlen(menu_items[i]) + bar_gap;
    }
}

static void add_habit(Habit *list, int *current_total) {
    if(*current_total >= max_habits_amount) return;

    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int height = 3;
    int width = name_max_length + 20;
    int start_y = (rows - height) / 2;
    int start_x = (cols - width) / 2;

    WINDOW *win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    
    box(win, 0, 0); 

    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 1, width - 9, "(<- Esc)");
    wattroff(win, COLOR_PAIR(3));
    mvwprintw(win, 1, 1, "Your habit: ");

    char temp_name[name_max_length] = {0};
    int char_count = 0;
    int ch;

    curs_set(1);

    while(1) {
        ch = wgetch(win);
        if(ch == key_escape) {
            curs_set(0);
            delwin(win);
            return;
        }
        else if(ch == key_enter) {
            if(char_count > 0) {
                strncpy(list[*current_total].name, 
                        temp_name, name_max_length - 1);
                list[*current_total].name[name_max_length - 1] = '\0';
            }
            break;
        }
        else if(ch == KEY_BACKSPACE && char_count > 0) {
            char_count--;
            temp_name[char_count] = '\0';
            int cur_y, cur_x;
            getyx(win, cur_y, cur_x);
            mvwaddch(win, cur_y, cur_x - 1, ' ');
            wmove(win, cur_y, cur_x - 1);
        }
        else if(ch >= 32 && ch <= 126 && 
                char_count < name_max_length - 1) {
            temp_name[char_count] = (char)ch;
            char_count++;
            waddch(win, ch);
        }
        wrefresh(win);
    }

    list[*current_total].last_done = 0;
    for(int i = 0; i < days_in_year; i++)
        list[*current_total].history[i] = false;
    (*current_total)++;

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

static void rename_habit(Habit *habit)
{
    clear();
    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int height = 5;
    int width = name_max_length + 20;
    int start_y = (rows - height) / 2;
    int start_x = (cols - width) / 2;

    WINDOW *win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    
    box(win, 0, 0); 

    mvwprintw(win, 1, 2, "Current name: %s", habit->name);
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 1, width - 9, "<- Esc");
    wattroff(win, COLOR_PAIR(3));

    mvwprintw(win, 3, 2, "New name: ");
    wrefresh(win);

    char temp_name[name_max_length] = {0};
    int char_count = 0;
    int ch;

    curs_set(1);

    while(1) {
        ch = wgetch(win);
        if(ch == key_escape) {
            curs_set(0);
            delwin(win);
            return;
        }
        else if(ch == key_enter) {
            if(char_count > 0) {
                strncpy(habit->name, temp_name, name_max_length - 1);
                habit->name[name_max_length - 1] = '\0';
            }
            break;
        }
        else if(ch == KEY_BACKSPACE && char_count > 0) {
            char_count--;
            temp_name[char_count] = '\0';
            int cur_y, cur_x;
            getyx(win, cur_y, cur_x);
            mvwaddch(win, cur_y, cur_x - 1, ' ');
            wmove(win, cur_y, cur_x - 1);
        }
        else if(ch >= 32 && ch <= 126 && 
                char_count < name_max_length - 1) {
            temp_name[char_count] = (char)ch;
            char_count++;
            waddch(win, ch);
        }
        wrefresh(win);
    }

    curs_set(0);
    delwin(win);

}

static void print_week_labels(int y, int x)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int today_wday = t->tm_wday + debug_day;

    const char days[] = {'S', 'M', 'T', 'W', 'T', 'F', 'S'};

    move(y, x + checkbox_offset);
    attron(COLOR_PAIR(3));
    for(int i = 0; i < days_in_week; i++) {
        if(i == days_in_week - 1) {
            attroff(COLOR_PAIR(3));
        }
        int idx = (today_wday - 
                (days_in_week - 1 - i) + days_in_week) % days_in_week;
        printw(" %c ", days[idx]);
        if(i == days_in_week - 1) {
            attron(COLOR_PAIR(3));
        }
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
    
    box(win, 0, 0); 

    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 1, 2, "<- Esc");
    wattroff(win, COLOR_PAIR(3));

    // 4. Draw Content
    wattron(win, A_BOLD);
    mvwprintw(win, 1, (width - 14) / 2, " CONFIRMATION "); // 14 here is the length of the string passed
    wattroff(win, A_BOLD);

    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 3, (width - 32) / 2,
            "Are you sure you want to delete:"); // 32 is the length of the string
    wattroff(win, COLOR_PAIR(3));
    
    // Highlight the habit name in Red to show it's the target of deletion
    mvwprintw(win, 4, (width - strlen(habit_name) - 2) / 2, "'%s'?", habit_name);

    // Drawing "Buttons"
    int btn_y = 6;
    
    wattron(win, COLOR_PAIR(3));
    mvwaddstr(win, btn_y, (width / 2) - 10, "[Y]es");
    wattroff(win, COLOR_PAIR(3));

    mvwaddstr(win, btn_y, (width / 2) + 5, "[N]o");

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

static void draw_calendar(Habit *h) {
    // 1. Setup Time Data
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

    int current_month = t->tm_mon; // 0-11
    int current_year = t->tm_year + 1900;
    int real_today = t->tm_mday;
    int view_day = real_today;

    // 2. Find out when the 1st of the month starts
    struct tm first_val = *t;
    first_val.tm_mday = 1;
    mktime(&first_val); // This auto-calculates wday and yday for the 1st

    int start_wday = first_val.tm_wday; // 0=Sun, 1=Mon...
    int start_yday = first_val.tm_yday; // 0-365 index in your history array

    // 3. Find days in this month
    // Trick: Go to day "0" of next month to find last day of current month
    struct tm last_val = *t;
    last_val.tm_mon++;     // Next month
    last_val.tm_mday = 0;  // "0th" day of next month is last day of this month
    mktime(&last_val);
    int days_in_month = last_val.tm_mday;

    while(1) {
        // 4. UI Setup
        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        erase(); // Clear screen

        // Calculate centering
        // Calendar is roughly 20 chars wide (7 days * 3 chars)
        int start_x = (cols - 22) / 2; 
        int start_y = (rows - 10) / 2;

        // 5. Draw Header
        mvprintw(start_y, start_x + 8, "%s %d", months[current_month], current_year);
        attron(COLOR_PAIR(3));
        mvprintw(start_y + 2, start_x + 1, "S  M  T  W  T  F  S");
        attroff(COLOR_PAIR(3));

        // 6. Draw Days
        int row = 0;
        int col = start_wday; // Start printing at the correct weekday column

        for (int day = 1; day <= days_in_month; day++) {
            int history_idx = start_yday + (day - 1);
            int ui_y = start_y + 3 + row;
            int ui_x = start_x + (col * 3);

            // Determine Color
            bool is_done = h->history[history_idx];
            bool to_view = (day == view_day);
            bool is_real_today = (day == real_today);

            if(to_view && is_real_today && is_done)
                attron(COLOR_PAIR(4));
            else if(to_view && is_real_today) 
                attron(COLOR_PAIR(7));
            else if(is_real_today && is_done)
                attron(COLOR_PAIR(2));
            else if(is_real_today)
                attron(COLOR_PAIR(5));
            else if (to_view && is_done)
                attron(COLOR_PAIR(8));
            else if (to_view)
                attron(A_REVERSE); // Just Highlighted
            else if (is_done)
                attron(A_NORMAL); // Cyan text
            else
                attron(COLOR_PAIR(3));

            mvprintw(ui_y, ui_x, "%2d", day);

            // Turn off attributes
            if(to_view && is_real_today && is_done) attroff(COLOR_PAIR(4));
            else if(to_view && is_real_today) attroff(COLOR_PAIR(7));
            else if(is_real_today && is_done) attroff(COLOR_PAIR(2));
            else if(is_real_today) attroff(COLOR_PAIR(5));
            else if (to_view && is_done) attroff(A_REVERSE | COLOR_PAIR(2));
            else if (to_view) attroff(A_REVERSE);
            else if (is_done) attroff(COLOR_PAIR(2));
            else attroff(COLOR_PAIR(3));

            // Move to next column
            col++;
            if (col > 6) { // Wrap to next week
                col = 0;
                row++;
            }
        }

        // Footer
        attron(COLOR_PAIR(3));
        mvprintw(start_y, start_x, "<- Esc");
        attroff(COLOR_PAIR(3));

        int total_done = 0;
        for(int day = 1; day <= days_in_month; day++) {
            int history_idx = start_yday + (day - 1);
            if(h->history[history_idx])
                total_done++;
        }

        attron(COLOR_PAIR(3));
        mvprintw(start_y + 8, start_x, "--------------------");
        mvprintw(start_y + 9, start_x, "Done: %d", total_done);
        attroff(COLOR_PAIR(3));
        refresh();

        int ch = getch();
        switch(ch) {
            case 'k':   
            case KEY_UP:
                if(view_day - days_in_week >= 1) view_day -= days_in_week;
                break;
            case 'j': 
            case KEY_DOWN:
                if(view_day + days_in_week <= days_in_month) view_day += days_in_week;
                break;
            case 'h': 
            case KEY_LEFT:
                view_day = (view_day - 1 + days_in_month) % days_in_month;
                if(!view_day) view_day = days_in_month;
                break;
            case 'l': 
            case KEY_RIGHT:
                view_day = (view_day + 1) % days_in_month;
                if(!view_day) view_day = days_in_month;
                break;
            case key_enter:
                mark_habit_done(h, view_day - 1); 
                break;
            case key_escape: 
                return;
        }
    }
}

static void draw_daily_progress(int rows, int cols, Habit *habits, int total, int view_day) {
    if (total == 0) return; // Prevent division by zero

    // 1. Calculate counts
    int completed = 0;
    
    // Handle index wrapping (just like in draw_habit_item)
    int safe_day_idx = view_day;
    if (safe_day_idx < 0) safe_day_idx += days_in_year;

    for (int i = 0; i < total; i++) {
        if (habits[i].history[safe_day_idx]) {
            completed++;
        }
    }

    // 2. Setup Dimensions
    // Width is screen width minus margins (let's say 4 chars padding)
    int bar_width = dashboard_length;
    
    int filled_len = (int)((float)completed / total * bar_width);

    // 3. Draw the Bar
    int x_pos = (cols / 2) - 24;
    int y_pos = (rows / 2) - (total / 2) - 2;
    
    move(y_pos, x_pos);

    // Draw Filled Portion (White)
    attron(COLOR_PAIR(9));
    for (int i = 0; i < filled_len; i++)
        addch('-'); 
    attroff(COLOR_PAIR(9));

    // Draw Empty Portion (Dimmed)
    attron(COLOR_PAIR(3)); 
    for (int i = filled_len; i < bar_width; i++)
        addch('-');
    attroff(COLOR_PAIR(3));

    // 4. Draw Text Overlay (Centered)
    char status[10];
    snprintf(status, sizeof(status), " %d%% ", (completed * 100) / total);
    
    if(completed != total) attron(COLOR_PAIR(3));
    else attron(COLOR_PAIR(9));

    mvprintw(y_pos, (cols - strlen(status)) / 2, "%s", status);
    
    if(completed == total) attroff(COLOR_PAIR(3));
    else attroff(COLOR_PAIR(9));
}

static void main_screen(Habit *habits, int *total) {
    int highlight = 0;
    time_t now = time(NULL);
    int real_today = localtime(&now)->tm_yday + debug_day;
    int view_day = real_today;

    while(1) {
        int r, c;
        getmaxyx(stdscr, r, c);
        
        // Logical centering
        int list_x = (c / 2) - 25;
        int list_y = (r / 2) - (*total / 2);

        erase();

        draw_daily_progress(r, c, habits, *total, view_day);


        if(*total == 0)
            mvprintw(list_y, list_x, "No habits found. Press (1) to add.");
        else {
            print_week_labels(list_y - 1, list_x);
            for(int i = 0; i < *total; i++)
                draw_habit_item(list_y + i, list_x, view_day, i == highlight, habits[i]);
        }

        action_bar(r, c);
        refresh();

        int ch = getch();
        switch(ch) {
            case 'k':   
            case KEY_UP:
                if(*total > 0) highlight = (highlight - 1 + *total) % *total; 
                break;
            case 'j': 
            case KEY_DOWN:
                if(*total > 0) highlight = (highlight + 1) % *total; 
                break;
            case 'h': 
            case KEY_LEFT:
                if(view_day > real_today - 6) view_day--; 
                break;
            case 'l': 
            case KEY_RIGHT:
                if(view_day < real_today) view_day++; 
                break;
            case '1': 
            case 'a':
                add_habit(habits, total); 
                break;
            case '2': 
            case 'd':
                if(*total > 0 && confirm_delete(habits[highlight].name)) {
                    delete_habit(highlight, habits, total);
                    if(highlight >= *total && highlight > 0) highlight--;
                }
                break;
            case '3': 
            case 'r':
                rename_habit(&habits[highlight]);
                break;
            case key_enter: 
            case 13: 
                if(*total > 0) mark_habit_done(&habits[highlight], view_day); 
                break;
            case '4':
            case 'c':
                if(*total > 0) draw_calendar(&habits[highlight]);
                break;
            case '5': 
            case 'q':
            case key_escape:
                upload_to_disk(habits, *total);
                endwin(); 
                exit(0);
        }
    }
}

static void load_habits(Habit *habits, int *current_total) {
    FILE *from = fopen(HABITS_FILE, "r");
    if(!from) return;
    char line[512];
    int i = 0;
    while(fgets(line, sizeof(line), from) && i < max_habits_amount) {
        char s[days_in_year + 1];
        if(sscanf(line, " %49[^,],%ld,%s", habits[i].name, &habits[i].last_done, s) == habit_fields) {
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
    int r, c;
    getmaxyx(stdscr, r, c);
    if(c < dashboard_length) {
        printw("The width of the screen is not enough for this program to run properly. Press any key to terminate.");
        getch();
        endwin();
        return 0;
    }
    cbreak();
    noecho();
    keypad(stdscr, 1);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_WHITE); 
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, 242, COLOR_BLACK); // Dimmed text
    init_pair(4, COLOR_GREEN, 242);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    init_pair(7, COLOR_RED, 242);
    init_pair(8, COLOR_WHITE, 242);
    init_pair(9, 250, COLOR_BLACK);
    curs_set(0);

    int total = 0;
    Habit my_habits[max_habits_amount];

    load_habits(my_habits, &total);
    main_screen(my_habits, &total);

    endwin();
    return 0;
}
