#include <curses.h>

const char new_habit_message[] = "Add new habit";
enum { key_escape = 27, key_enter = 10 };

static void colors_init()
{
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
}

static int start_menu(int start_x, int start_y)
{
    static const char *menu_items[] = {
        "Add new habit",
        "Settings",
        "Exit"
    };
    int n_choices = sizeof(menu_items) / sizeof(char *); 
    int key;
    int highlight = 0; // Index of the currently selected item

    while(1) {
        // 1. Draw all items
        for(int i = 0; i < n_choices; i++) {
            if(i == highlight) {
                attron(COLOR_PAIR(1) | A_REVERSE); // Highlighted style
            }

            mvaddstr(start_y + i, start_x, menu_items[i]);

            if(i == highlight) {
                attroff(COLOR_PAIR(1) | A_REVERSE);
            }
        }
        refresh();

        // 2. Handle Input
        key = getch();
        switch(key) {
            case KEY_UP:
                highlight--;
                if(highlight < 0) highlight = n_choices - 1; // Wrap to bottom
                break;
            case KEY_DOWN:
                highlight++;
                if(highlight >= n_choices) highlight = 0;    // Wrap to top
                break;
            case 10: // Enter key (often represented as 10 or KEY_ENTER)
                return highlight; // Return which index was chosen
        }
        
        if (key == key_escape) break;
    }
}

int main() {
    int row, col, key;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, 1);
    timeout(100);
    getmaxyx(stdscr, row, col);
    curs_set(0);
    colors_init();

    start_menu(col/2, row/2);
    while((key = getch()) != key_escape)
    {
    }
    endwin();
    return 0;
}
