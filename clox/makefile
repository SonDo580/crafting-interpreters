# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -g

# Source files
SRC = main.c chunk.c memory.c debug.c value.c

# Object files (derived from source files)
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = clox.o

# Default target
all: $(EXEC)

# Link the object files to create the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJ) $(EXEC)

# Phony targets (not actual files)
.PHONY: all clean
