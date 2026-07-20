#include "save.h"
#include <stdlib.h>
#include <string.h>

static void get_data_path(char *dest) {
    const char *home = getenv("HOME");
    if(home == NULL)
        strncpy(dest, HABITS_FILE, PATH_MAX);
    else
        snprintf(dest, PATH_MAX, "%s/%s", home, HABITS_FILE);
}

void upload_to_disk(Habit *habits, int current_total) {
    char path[PATH_MAX];
    get_data_path(path);

    FILE *dest = fopen(path, "w");
    if(!dest) return;

    for(int i = 0; i < current_total; i++) {
        fprintf(dest, "%s,%ld,%d,", 
                habits[i].name, 
                habits[i].last_done, 
                habits[i].year);
        for(int j = 0; j < days_in_year; j++)
            fputc(habits[i].history[j] ? '1' : '0', dest);
        fputc('\n', dest);
    }
    fclose(dest);
}

void load_habits(Habit *habits, int *current_total) {
    enum {
        habit_fields = 4,
    };
    char path[PATH_MAX];
    get_data_path(path);

    FILE *from = fopen(path, "r");
    if(!from) return;
    char line[512];
    int i = 0;

    char fmt[64];
    char s_fmt[20];

    time_t now = time(NULL);
    int current_year = localtime(&now)->tm_year + 1900;

    snprintf(s_fmt, sizeof(s_fmt), "%%%ds", days_in_year);
    snprintf(fmt, sizeof(fmt), " %%%d[^,],%%ld,%%d,%s", name_max_length - 1, s_fmt);

    while(fgets(line, sizeof(line), from) && i < max_habits_amount) {
        char s[days_in_year + 1];
        if(sscanf(line, fmt, habits[i].name, &habits[i].last_done, &habits[i].year, s) == habit_fields) {
            if(habits[i].year != current_year) {
                memset(habits[i].history, 0, sizeof(habits[i].history));
                habits[i].year = current_year;
            } else
                for(int j = 0; j < days_in_year; j++)
                    habits[i].history[j] = (s[j] == '1');
            i++;
        }
            
    }
    *current_total = i;
    fclose(from);
}
