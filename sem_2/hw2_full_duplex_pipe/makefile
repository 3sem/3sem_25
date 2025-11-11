PROGRAM = duplex_pipe

all: src/main.c src/pipe.c
	gcc -Wall -Iinclude -o $(PROGRAM) src/main.c src/pipe.c

clean:
	rm -f $(PROGRAM)

test: $(PROGRAM)
	@echo "Testing with big file..."
	dd if=/dev/urandom of=test bs=1048576 count=4096
	./$(PROGRAM) test
	@md5sum test test_copy
	@rm -f test test_copy

test-small: $(PROGRAM)
	@echo "Testing with small file..."
	dd if=/dev/urandom of=test_small bs=1M count=10
	./$(PROGRAM) test_small
	@md5sum test_small test_small_copy
	@rm -f test_small test_small_copy

help:
	@echo "Available targets:"
	@echo "  all        - Build the program (default)"
	@echo "  test       - Test with small file (4GB)"
	@echo "  test-small - Test with small file (10MB)"
	@echo "  clean      - Remove program"
	@echo "  help       - Show this help"