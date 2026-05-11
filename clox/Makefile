TARGET = clox
CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g
SRCS = $(wildcard *.c)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
