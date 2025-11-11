#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SEM_SENDER_KEY 0x1
#define SEM_RECEIVER_KEY 0x2
#define SHM_SIZE 128*1024
#define SHM_KEY 0x8

#define IN_FILE_NAME "input_file.txt"
#define OUT_FILE_NAME "output_file.txt"

typedef struct 
{
    size_t file_size;    
    size_t data_size; 
} shm_file_t;


int sem_ctor(int key, int initial_value);
void sem_wait(int semid);
void sem_sig(int semid);
void sem_dtor(int semid);