CC = gcc
CFLAGS = -Wall -Wextra -std=c99

TARGET = prog
TEST_FILE = file.txt
RESULT_FILE = output.txt
SRC = hw2.c

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SRC)
	@$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	@./$(TARGET)

test: $(TARGET)
	@time dd if=/dev/urandom of=file.txt bs=1048576 count=4096 > /dev/null 2>&1

	@./$(TARGET)
	@RES_1=`md5sum $(TEST_FILE) | cut -d' ' -f1`; \
	RES_2=`md5sum $(RESULT_FILE) | cut -d' ' -f1`; \
	echo "$(TEST_FILE) md5 sum: $$RES_1"; \
	echo "$(RESULT_FILE) md5 sum: $$RES_2"; \
	if [ "$$RES_1" = "$$RES_2" ]; then \
		echo "all right, md5 sums are correct"; \
	else \
		echo "not ok"; \
		exit 1; \
	fi

clean:
	rm -f $(TARGET) $(TEST_FILE) $(RESULT_FILE)