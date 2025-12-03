TARGET = echo
SRC = echo.c
CC = gcc
CFLAGS = -O2

# Test files
TESTFILE = bigfile.bin
RECEIVED = received.bin

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Create 4GB random file
$(TESTFILE):
	@echo "Creating 4GB test file..."
	@dd if=/dev/urandom of=$(TESTFILE) bs=1M count=4096 status=progress

# Run
test: $(TARGET) $(TESTFILE)
	@echo "Running echo test..."
	./$(TARGET) $(TESTFILE) $(RECEIVED)
	@echo "Verifying md5sum..."
	@md5sum $(TESTFILE) $(RECEIVED)

# Cleanup
clean:
	rm -f $(TARGET) $(RECEIVED) $(TESTFILE)