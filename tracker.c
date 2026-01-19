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
    idx_settings,
    idx_delete,
    idx_rename,
    idx_calendar,
    idx_exit,
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
        attron(A_DIM); 
        printw("  -  ");
        attroff(A_DIM);
    } 
    else if (streak < days_in_week) {
        // State: Spark (Yellow, standard weight)
        attron(COLOR_PAIR(8)); // Yellow
        printw(" %d ", streak);
        attroff(COLOR_PAIR(8));
    } 
    else {
        // State: ON FIRE (Red, Bold)
        attron(COLOR_PAIR(7) | A_BOLD); // Red + Bold
        printw(" %d ", streak);
        attroff(COLOR_PAIR(7) | A_BOLD);
    }
    // --- IMPROVED STREAK UI END ---

    // The rest of your code works perfectly with this because 
    // it calculates padding dynamically!
    
    int cur_y, cur_x;
    printw("%s", habit.name);

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
        
        char c = habit.history[history_idx] ? 'x' : ' ';
        
        // Use A_BOLD on the highlight to make the cursor 'pop' more
        if(wd == target_column && highlighted) attron(COLOR_PAIR(2) | A_BOLD);
        printw("[%c]", c);
        if(wd == target_column && highlighted) {
            attroff(COLOR_PAIR(2) | A_BOLD);
            // Restore normal row highlight if needed
            attron(COLOR_PAIR(1)); 
        }
    }
}

static void draw_habit_item_two(int y, int x, int selected_yday, bool highlighted, Habit habit) {
    time_t now = time(NULL);
    int real_today = localtime(&now)->tm_yday + debug_day;

    int day_offset = real_today - selected_yday;
    int target_column = days_in_week - 1 - day_offset;

    int streak = get_streak(habit, real_today);

    mvprintw(y, x, "[%d] ", streak);

    int cur_y, cur_x;

    printw("%s", habit.name);

    int checkbox_start_col = x + checkbox_offset;
    getyx(stdscr, cur_y, cur_x);
    while(cur_x < checkbox_start_col) {
        addch(' ');
        cur_x++;
    }

    for(int wd = 0; wd < days_in_week; wd++) {
        int history_idx = real_today - (days_in_week - 1 - wd);
        if(history_idx < 0) 
            history_idx += days_in_year;
        char c = habit.history[history_idx] ? 'x' : ' ';

        if(wd == target_column && highlighted) attron(COLOR_PAIR(2));
        printw("[%c]", c);
        if(wd == target_column && highlighted) {
            attroff(COLOR_PAIR(2));
            attron(COLOR_PAIR(1));
        }
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
        "(1) Add habit",
        "(2) Settings",
        "(3) Delete",
        "(4) Rename",
        "(5) Calendar",
        "(6) Exit"
    };
    int total_width = 0;
    for(int i = 0; i < menu_count; i++) total_width += strlen(menu_items[i]) + bar_gap;

    // Center the bar at the bottom of the screen
    int x_offset = (cols - total_width) / 2;
    int y_pos = rows - 2;

    attron(COLOR_PAIR(2));
    // Draw a background strip for the menu
    mvhline(y_pos, 0, ' ', cols); 
    
    for(int i = 0; i < menu_count; i++) {
        mvaddstr(y_pos, x_offset, menu_items[i]);
        x_offset += strlen(menu_items[i]) + bar_gap;
    }
    attroff(COLOR_PAIR(2));
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
    
    wbkgd(win, COLOR_PAIR(3));
    box(win, 0, 0); 

    mvwprintw(win, 1, 1, "Your habit: ");
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
                strncpy(list[*current_total].name, temp_name, name_max_length - 1);
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
        else if(ch >= 32 && ch <= 126 && char_count < name_max_length - 1) {
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
    
    wbkgd(win, COLOR_PAIR(3));
    box(win, 0, 0); 

    mvwprintw(win, 1, 2, "Current name: %s", habit->name);

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
        else if(ch >= 32 && ch <= 126 && char_count < name_max_length - 1) {
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

    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

    move(y, x + checkbox_offset);
    for(int i = 0; i < days_in_week; i++) {
        if(i == days_in_week - 1)
            attron(COLOR_PAIR(7));
        int idx = (today_wday - (days_in_week - 1 - i) + days_in_week) % days_in_week;
        printw("%s ", days[idx]);
        if(i == days_in_week - 1)
            attroff(COLOR_PAIR(7));
    }
    printw("  <-- the red one is today");
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
    wattron(win, COLOR_PAIR(7));
    mvwprintw(win, 4, (width - strlen(habit_name) - 2) / 2, "'%s'?", habit_name);
    wattroff(win, COLOR_PAIR(7));

    // Drawing "Buttons"
    int btn_y = 6;
    
    // The "Cancel" option (Neutral/Minimalist)
    mvwaddstr(win, btn_y, (width / 2) + 2, "[N]o");

    // The "Delete" option (Highlighted in Red)
    wattron(win, COLOR_PAIR(7));
    mvwaddstr(win, btn_y, (width / 2) - 12, "[Y]es");
    wattroff(win, COLOR_PAIR(7));

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
    int today_mday = t->tm_mday;

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

    // 4. UI Setup
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase(); // Clear screen

    // Calculate centering
    // Calendar is roughly 20 chars wide (7 days * 3 chars)
    int start_x = (cols - 22) / 2; 
    int start_y = (rows - 10) / 2;

    // 5. Draw Header
    attron(A_BOLD);
    mvprintw(start_y, start_x + 2, "%s %d", months[current_month], current_year);
    //mvprintw(start_y, start_x, "   %s %d", "Month", current_year); // You can add month names array if you want
    mvprintw(start_y + 2, start_x, "Su Mo Tu We Th Fr Sa");
    attroff(A_BOLD);

    // 6. Draw Days
    int row = 0;
    int col = start_wday; // Start printing at the correct weekday column

    for (int day = 1; day <= days_in_month; day++) {
        int history_idx = start_yday + (day - 1);
        int ui_y = start_y + 3 + row;
        int ui_x = start_x + (col * 3);

        // Determine Color
        // If done: Green (Pair 4 or 6)
        // If today: Bold/White (Pair 2 or standard)
        // If missed: Dim or default
        
        bool is_done = h->history[history_idx];
        bool is_today = (day == today_mday);

        if (is_today && is_done) {
            attron(A_REVERSE | COLOR_PAIR(4)); // Highlighted Green
        } else if (is_today) {
            attron(A_REVERSE); // Just Highlighted
        } else if (is_done) {
            attron(COLOR_PAIR(4)); // Green text
        }

        mvprintw(ui_y, ui_x, "%2d", day);

        // Turn off attributes
        if (is_today && is_done) attroff(A_REVERSE | COLOR_PAIR(4));
        else if (is_today) attroff(A_REVERSE);
        else if (is_done) attroff(COLOR_PAIR(4));

        // Move to next column
        col++;
        if (col > 6) { // Wrap to next week
            col = 0;
            row++;
        }
    }

    // Footer
    mvprintw(start_y + 3 + row + 2, start_x - 4, "Press any key to return");
    refresh();
    getch(); // Wait for user input
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

        attron(A_BOLD | A_UNDERLINE);
        mvprintw(list_y - 4, list_x, "HABITS TRACKER");
        attroff(A_BOLD | A_UNDERLINE);

        if(*total == 0) {
            mvprintw(list_y, list_x, "No habits found. Press (1) to add.");
        } else {
            print_week_labels(list_y - 1, list_x);
            for(int i = 0; i < *total; i++) {
                if(i == highlight) attron(COLOR_PAIR(1));
                draw_habit_item(list_y + i, list_x, view_day, i == highlight, habits[i]);
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
                add_habit(habits, total); 
                break;
            case '3': 
                if(*total > 0 && confirm_delete(habits[highlight].name)) {
                    delete_habit(highlight, habits, total);
                    if(highlight >= *total && highlight > 0) highlight--;
                }
                break;
            case '4': 
                rename_habit(&habits[highlight]);
                break;
            case key_enter: 
            case 13: 
                if(*total > 0) mark_habit_done(&habits[highlight], view_day); 
                break;
            case '5':
                if(*total > 0) draw_calendar(&habits[highlight]);
                break;
            case '6': 
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
    cbreak();
    noecho();
    keypad(stdscr, 1);
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK); 
    init_pair(2, COLOR_BLACK, COLOR_WHITE); 
    init_pair(3, COLOR_WHITE, COLOR_BLACK); // Highlighted cell
    init_pair(4, COLOR_GREEN, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, 235); 
    init_pair(6, COLOR_WHITE, COLOR_GREEN);
    init_pair(7, COLOR_RED, COLOR_BLACK);
    init_pair(8, COLOR_YELLOW, COLOR_BLACK);
    curs_set(0);

    int total = 0;
    Habit my_habits[max_habits_amount];

    load_habits(my_habits, &total);
    main_screen(my_habits, &total);

    endwin();
    return 0;
}
