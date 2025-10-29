#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>

#include "fifo_send.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("wrong file configuration for fifo test\n");
        return -1;
    }
    char *src_file = argv[1];
    char *dst_file = argv[2];

    printf("started from file %s to file%s\n", src_file, dst_file);

    Fifo *fifo = ctor_fifo();
    printf("buf %p fd %d name %s size%d\n",
         fifo->buffer, fifo->fd, fifo->name, fifo->size);
    fifo->op.open(fifo);

    printf("fifo created and opened\n");

    struct timeval time_start, time_end, prog_time;

    gettimeofday(&time_start, NULL);

    send_file_fifo(fifo, src_file, dst_file);

    gettimeofday(&time_end, NULL);

    dtor_fifo(fifo);


    timersub(&time_end, &time_start, &prog_time);
    printf("\x1b[32m" "Time use to sent file %s using shared memory: %ld s %ld us\n\n" "\x1b[0m",
           src_file, (uint64_t)prog_time.tv_sec, (uint64_t)prog_time.tv_usec);
}
