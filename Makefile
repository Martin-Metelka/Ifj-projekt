
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic

TARGET = ifj25-compiler
SOURCES = main.c scanner.c parser.c symtable.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = scanner.h parser.h symtable.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Testing compiler..."
	@./$(TARGET) < test_input.ifj25 > test_output.ifjcode25

.PHONY: all clean test