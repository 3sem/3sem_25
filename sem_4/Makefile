CC = gcc
CFLAGS = -O2 -Wall -pthread -lrt

# имена исполняемых файлов
PROGS = progA progB

all: $(PROGS)

progA: progA.c
	$(CC) $(CFLAGS) $< -o $@

progB: progB.c
	$(CC) $(CFLAGS) $< -o $@

run:
	@echo "Запуск progA (пример: 8 потоков, 10000000 точек)"
	./progA 8 10000000

clean:
	rm -f $(PROGS)
	rm -f /dev/shm/mc_shared
