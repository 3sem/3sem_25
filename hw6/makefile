CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC_DIR = src
INCLUDE_DIR = include

SOURCES = $(SRC_DIR)/process.c $(SRC_DIR)/daemon.c $(SRC_DIR)/fifo.c
HEADERS = $(INCLUDE_DIR)/process.h $(INCLUDE_DIR)/daemon.h $(INCLUDE_DIR)/fifo.h

all: daemon client

daemon: main.c $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ main.c $(SOURCES)

client: client.c $(HEADERS)
	$(CC) $(CFLAGS) -o $@ client.c

clean:
	rm -f daemon client
	rm -rf backups

.PHONY: all clean