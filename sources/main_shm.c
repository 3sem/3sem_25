#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "shared_mem_send.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("wrong file configuration for sared memory test\n");
        return -1;
    }
    char *src_file = argv[1];
    char *dst_file = argv[2];
    
    Shmem *shared_memory = ctor_shm();
    shared_memory->op.open(shared_memory);
    // printf("shared memory created and opened\n");
    
    struct timeval time_start, time_end, prog_time;
    gettimeofday(&time_start, NULL);

    shared_memory->op.send_file(shared_memory, src_file, dst_file);
    
    gettimeofday(&time_end, NULL);

    // printf("shared memory, %s sended to %s\n", src_file, dst_file);

    shared_memory->op.destroy(shared_memory);

    // printf("shared_memory closed and destroyed\n");

    timersub(&time_end, &time_start, &prog_time);
    printf("\x1b[32m" "Time use to sent file %s using shared memory: %ld s %ld us\n\n" "\x1b[0m",
           src_file, (uint64_t)prog_time.tv_sec, (uint64_t)prog_time.tv_usec);
}