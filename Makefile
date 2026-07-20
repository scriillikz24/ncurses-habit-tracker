CC=gcc
CFLAGS=-Wall -g -Iinclude
LDFLAGS=-lcurses
TARGET=tracker

OBJ = \
	  src/main.o \
	  src/colors.o \
	  src/habit.o \
	  src/save.o \
	  src/ui.o \

prog: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o tracker

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o
