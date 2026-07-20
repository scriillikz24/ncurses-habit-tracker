#include "habit.h"

#ifndef SAVE_H
#define SAVE_H

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

void upload_to_disk(Habit *habits, int current_total);

void load_habits(Habit *habits, int *current_total);

#endif
