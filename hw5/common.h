#ifndef SHARED_H
#define SHARED_H

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME "shm"
#define BUFFER_SIZE 4096

struct shm_header 
{
    pid_t sender_pid;
    pid_t receiver_pid;
    size_t data_size;
};

#define IN_FILE_NAME "input_file.txt"
#define OUT_FILE_NAME "output_file.txt"

#define MAX_DATA_SIZE (BUFFER_SIZE - sizeof(struct shm_header))

#endif