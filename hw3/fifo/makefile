CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE
TARGETS = sender receiver
TEST_FILE = input_file.txt
RESULT_FILE = output_file.txt
TEST_FILE_SIZE = 4096

all: $(TARGETS)

sender: snd.c common.h
	$(CC) $(CFLAGS) -o sender snd.c

receiver: rcv.c common.h
	$(CC) $(CFLAGS) -o  receiver rcv.c

run:
	@$(CC) $(CFLAGS) -o sender snd.c
	@$(CC) $(CFLAGS) -o  receiver rcv.c
	@time dd if=/dev/urandom of=$(TEST_FILE) bs=1048576 count=$(TEST_FILE_SIZE) > /dev/null 2>&1
	(./receiver &) && sleep 2 && ./sender
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
	rm -f $(TARGETS)
	rm -f $(TEST_FILE) 
	rm -f $(RESULT_FILE)

.PHONY: all clean