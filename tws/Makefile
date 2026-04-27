CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

SRC = src/main.c src/server.c src/http_parser.c src/logger.c
TARGET = tws

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

test:
	bash tests/test_basic.sh

clean:
	rm -f $(TARGET)