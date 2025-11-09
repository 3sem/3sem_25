CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic
LDFLAGS = -lpthread -lm -lrt

TARGET_A = A
TARGET_B = B
LOG_FILE = output_b.log
TARGETS = $(TARGET_A) $(TARGET_B)

all: $(TARGETS)

$(TARGET_A): A.c
	$(CC) $(CFLAGS) A.c -o $@ $(LDFLAGS)

$(TARGET_B): B.c
	$(CC) $(CFLAGS) B.c -o $@ $(LDFLAGS)

run: all
	@./$(TARGET_B) > $(LOG_FILE) 2>&1 &
	@sleep 1
	@./$(TARGET_A) 10 10000000 0.0 1.0 1.0
	@wait
	@cat $(LOG_FILE);

clean:
	rm -f $(TARGETS) $(LOG_FILE) $(LOG_FILE).*

.PHONY: all run clean