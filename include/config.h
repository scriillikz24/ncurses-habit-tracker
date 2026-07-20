#ifndef CONFIG_H
#define CONFIG_H

#define ESC_HINT "<- Esc"
#define HABITS_FILE ".habits.csv"

enum {
    days_in_week = 7,
    days_in_year = 366,
    name_max_length = 25,
    max_habits_amount = 10,
    esc_hint_length = 6,
    checkbox_offset = 30,
    key_escape = 27,
    key_enter = 10,
    key_tab = 9,
    bar_gap = 4,
    dashboard_length = 49,
};

#endif
