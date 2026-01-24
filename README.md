# Terminal Habit Tracker
A lightweight, distraction-free **Habit Tracker** built in C using the ncurses library. Keep track of your daily goals, monitor your streaks, and visualize your progress directly from your terminal.

## Features
- **TUI Dashboard**: A clean, color-coded interface for managing your daily tasks.
- **Streak Tracking**: Automatic calculation of current streaks with visual indicators (Yellow for active, Bold Red for 7+ days).
- **Calendar View**: A detailed monthly view to see your full history and toggle past completions.
- **Persistence**: Data is automatically saved to a hidden .habits.csv file in your directory.
- **Vim-Style Navigation**: Support for both Arrow Keys and `hjkl` navigation.
- **Lightweight**: Minimal dependencies and lightning-fast execution.

## Installation & Compilation
### 1.Prerequisites
You will need a C compiler (like `gcc`) and the `ncurses` development library.
#### On Arch:
``` bash
sudo pacman -S ncurses
```
### 2.Get the Source Code
Clone the repository or create the file manually:
``` bash
# Clone the repo
git clone https://github.com/scriillikz24/ncurses-habit-tracker.git
cd ncurses-habit-tracker
```
*Or, if you are creating it from scratch, save the code into a file named `main.c`.*
### 3.Compiling the Program
Run the following command in your terminal to compile:
``` bash
gcc -g main.c -lcurses -o habit_tracker

```
## Usage
Run the program with:
``` bash
./habit_tracker
```
### Keyboard Shortcuts
The tracker is designed for efficiency. Use the following keys:
- 1 or 'a': **Add** a new habit (Max 5)
- 2 or 'd': **Delete** selected habit (with confirmation)
- 3 or 'r': **Rename** selected habit
- 4 or c: Open **Calendar View** for the selected habit
- 5, 'q', or Esc: **Save & Exit**
- Enter: Toggle habit status for the selected day
- Arrows / hjkl: Navigate between habits and days

## Data Storage
Your data is stored in `.habits.csv`. The format is:`Name, Last_Done_Timestamp, Binary_History_String`
This allows you to easily back up your data or even script external tools to read your progress.


## Configuration
You can modify the constants at the top of the source code to customize your experience:
- `max_habits_amount`: Increase this if you have more than 5 habits.
- `name_max_length`: Change the maximum length of habit names.
- `debug_day`: Adjust this to simulate different days for testing purposes.
