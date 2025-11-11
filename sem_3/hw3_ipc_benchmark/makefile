PROGRAM = ipc_benchmark

help:
	@echo "-----COMMON TARGETS-----"
	@echo "  all         - Build the program"
	@echo "  clean       - Remove program"
	@echo "  help        - Show this help"
	@echo " "
	@echo "-----TESTING TARGETS-----"
	@echo "  shm_small   - testing shm transfer on a small file  (4 MB)"
	@echo "  shm_medium  - testing shm transfer on a medium file (32 MB)"
	@echo "  shm_big     - testing shm transfer on a big file    (512 MB)"
	@echo "  mq_small    - testing shm transfer on a small file  (4 MB)"
	@echo "  mq_medium   - testing shm transfer on a medium file (32 MB)"
	@echo "  mq_big      - testing shm transfer on a big file    (512 MB)"
	@echo "  fifo_small  - testing shm transfer on a small file  (4 MB)"
	@echo "  fifo_medium - testing shm transfer on a medium file (32 MB)"
	@echo "  fifo_big    - testing shm transfer on a big file    (512 MB)"

build: src/main.c src/fifo_transfer.c src/mq_transfer.c src/shm_transfer.c src/timing.c
	gcc -Wall -Iinclude -o $(PROGRAM) src/main.c src/fifo_transfer.c src/mq_transfer.c src/shm_transfer.c src/timing.c

clean:
	rm -f $(PROGRAM)

shm_small: $(PROGRAM)
	@echo "Testing with small file..."
	dd if=/dev/urandom of=test bs=1M count=1
	./$(PROGRAM) shm test
	@md5sum test copy_test
	@rm -f test copy_test

shm_medium: $(PROGRAM)
	@echo "Testing with medium file..."
	dd if=/dev/urandom of=test bs=1M count=32
	./$(PROGRAM) shm test
	@md5sum test copy_test
	@rm -f test copy_test

shm_big: $(PROGRAM)
	@echo "Testing with big file..."
	dd if=/dev/urandom of=test bs=1M count=512
	./$(PROGRAM) shm test
	@md5sum test copy_test
	@rm -f test copy_test

mq_small: $(PROGRAM)
	@echo "Testing with small file..."
	dd if=/dev/urandom of=test bs=1M count=1
	./$(PROGRAM) mq test
	@md5sum test copy_test
	@rm -f test copy_test

mq_medium: $(PROGRAM)
	@echo "Testing with medium file..."
	dd if=/dev/urandom of=test bs=1M count=32
	./$(PROGRAM) mq test
	@md5sum test copy_test
	@rm -f test copy_test

mq_big: $(PROGRAM)
	@echo "Testing with big file..."
	dd if=/dev/urandom of=test bs=1M count=512
	./$(PROGRAM) mq test
	@md5sum test copy_test
	@rm -f test copy_test

fifo_small: $(PROGRAM)
	@echo "Testing with small file..."
	dd if=/dev/urandom of=test bs=1M count=1
	./$(PROGRAM) fifo test
	@md5sum test copy_test
	@rm -f test copy_test

fifo_medium: $(PROGRAM)
	@echo "Testing with medium file..."
	dd if=/dev/urandom of=test bs=1M count=32
	./$(PROGRAM) fifo test
	@md5sum test copy_test
	@rm -f test copy_test

fifo_big: $(PROGRAM)
	@echo "Testing with big file..."
	dd if=/dev/urandom of=test bs=1M count=512
	./$(PROGRAM) fifo test
	@md5sum test copy_test
	@rm -f test copy_test