#ifndef SHM_TRANSFER_H_
#define SHM_TRANSFER_H

#include "common.h"
#include <semaphore.h>

#define SEM_READY_NAME "/ready_sem"
#define SEM_DONE_NAME "/processed_sem"

typedef struct {
    size_t  bytes_used;
    int     transfer_complete;  
    char    data[BUF_SIZE];
} shared_mem_t;

void shm_copy_file (const char *filename);

#endif