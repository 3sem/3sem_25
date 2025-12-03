CC = gcc
threads = 4
points = 10000000

build:
	$(CC) -o program_a program_a.c -lpthread -lrt -lm
	$(CC) -o program_b program_b.c -lrt -lm

run: program_a program_b
	@./program_a $(threads) $(points) &
	@sleep 2
	@./program_b
	@wait

clean:
	rm -f program_a program_b