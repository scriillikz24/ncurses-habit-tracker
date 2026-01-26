CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lcurses
TARGET=habits
SRC=tracker.c

# Default 'make' command - just compiles locally
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET) 

# Only run this when you want to update the "system-wide" version
install: all
	sudo cp $(TARGET) /usr/local/bin/$(TARGET)

# Useful for a fresh start
clean:
	rm -f $(TARGET)

# The "Dev Trick": Compile and Run in one command
run: all
	./$(TARGET)
