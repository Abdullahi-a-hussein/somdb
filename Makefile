CC = cc
CFLAGS = -Wall -Wextra -Werror -std=c11 -Iinclude

SRC = src/main.c src/value.c src/schema.c src/row.c src/table.c src/db.c src/storage.c src/parser.c src/repl.c
OBJ = $(SRC:.c=.o)
TARGET = somdb

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
