#ifndef THREAD_UTILS
#define THREAD_UTILS

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Debug_printf.h"

#define MIN_THREADS_INIT 1
#define MIN_THREADS 4
#define MAX_THREADS 16

typedef struct {
    pthread_t thread_id;
    int thread_num;
    char is_working; 
    char is_created;
    char end_flag;
    
    char filename[256];
    int client_id;
    int client_tx_fd;
    
    sem_t task_sem;
    
    int tasks_completed;
} tx_thread_t;

typedef struct {
    tx_thread_t threads[MAX_THREADS];
    int threads_count;
    int active_threads;

    pthread_mutex_t pool_mutex;
    sem_t available_threads; 
} tx_threads_pool_t;

extern tx_threads_pool_t* thread_pool;

tx_threads_pool_t* tx_pool_init(int threads_count);
void tx_pool_destroy(tx_threads_pool_t* pool);
int tx_pool_submit_task(tx_threads_pool_t* pool, int client_id, int client_tx_fd, const char* filename);
void* tx_thread_worker(void* arg);
int find_free_thread_slot(tx_threads_pool_t* pool);
tx_thread_t* find_available_thread(tx_threads_pool_t* pool);
tx_thread_t* create_thread(tx_threads_pool_t* pool);
int send_file_to_client(int client_tx_fd, const char* filename);

#endif
