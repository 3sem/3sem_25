#ifndef HANDLERS
#define HANDLERS

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <stdbool.h>
#include <time.h>
#include "Debug_printf.h"

#define CHUNKS 8
static const int Shm_size = 1024 * 1024;
static const int Chunk_size = Shm_size / CHUNKS;

#define SIG_PROD (SIGRTMIN + 2)
#define SIG_CONS (SIGRTMIN + 1)
#define SIG_EXIT (SIGRTMIN)

enum End_code {

    NOT_ENDED = 0x0,
    PROD_END_SUCC = 0x1,
    PROD_END_FAIL = 0x2,
    CONS_END_SUCC = 0x4,
    CONS_END_FAIL = 0x8
};

struct Chunk_data {

    char* chunk;
    int chunk_size;
};

struct Shared_data {

    char* buffer;
    size_t buf_size;
    char* producer_chunks[CHUNKS];    
    struct Chunk_data consumer_chunks[CHUNKS];    
    pid_t pid;
    pid_t ppid;
    int producer_ended;
    int consumer_ended;
};


void sig_exit_handler(int sig);
void producer_handler(int sig, siginfo_t *info, void *ucontext);
void consumer_handler(int sig, siginfo_t *info, void *ucontext);
#endif

