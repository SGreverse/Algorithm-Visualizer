CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinclude

# Flags required to link Raylib on Linux
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
EXEC = visualizer

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(EXEC)