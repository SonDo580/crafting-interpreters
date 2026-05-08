# Name of final program
TARGET = clox

# Compiler to use
CC = gcc

# Compiler flags:
# all warnings, even more warnings, debug info
CFLAGS = -Wall -Wextra -g

# Find all .c files and turn them into .o (object) files
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# Run when type 'make'
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Run when type 'make clean'
# Remove the executable and object files
clean:
	rm -f $(TARGET) $(OBJS)
