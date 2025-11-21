CC = gcc
CFLAGS = -O2
TARGETS = shm mq fifo

all: $(TARGETS)

shm_transfer: shm.c
	$(CC) $(CFLAGS) -o $@ $<

mq_transfer: mq.c
	$(CC) $(CFLAGS) -o $@ $<

fifo_transfer: fifo.c
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -f $(TARGETS) out_*.bin testfile.bin /tmp/md5_*.txt /tmp/ipc_fifo_test