CC=gcc
CFLAGS=-Wall -Wextra
TARGET=file_transfer
SOURCE=file_transfer.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

test:
	dd if=/dev/urandom of=test.txt bs=1024 count=64
	@echo "----------------------------"
	@echo "Started transfering..."
	@./$(TARGET) test.txt
	@echo "Transfering completed!"
	@echo "----------------------------"
	md5sum test.txt copy_test.txt
	@diff test.txt copy_test.txt && echo "Файлы идентичны!" || echo "Файлы разные!"
	@rm -f test.txt copy_test.txt

clean:
	rm -f $(TARGET)
