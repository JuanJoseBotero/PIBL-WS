CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

# PIBL.c includes the other .c files directly (Unity Build style)
SRC = PIBL.c
TARGET = pibl

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test:
	@echo "No tests defined for pibl yet."

clean:
	rm -f $(TARGET)
