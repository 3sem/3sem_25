#include "thread_utils.h"

extern tx_threads_pool_t* thread_pool;

tx_threads_pool_t* tx_pool_init(int threads_count) {
    if (threads_count > MAX_THREADS) {
        threads_count = MAX_THREADS;
    }
    
    tx_threads_pool_t* pool = (tx_threads_pool_t*) calloc(1, sizeof(tx_threads_pool_t));
    if (!pool) {
        DEBUG_PRINTF("ERROR: memory was not allocated\n");
        return NULL;
    }

    pool->threads_count = threads_count;
    pool->active_threads = 0;
    
    pthread_mutex_init(&pool->pool_mutex, NULL);
    sem_init(&pool->available_threads, 0, threads_count);
    
    for (int i = 0; i < threads_count; i++) {
        tx_thread_t* thread = &pool->threads[i];
        
        thread->thread_num = i;
        thread->is_working = 0;
        thread->is_created = 1;
        thread->end_flag = 0;
        thread->tasks_completed = 0;
        thread->client_id = -1;
        thread->client_tx_fd = -1;
        thread->filename[0] = '\0';
        sem_init(&thread->task_sem, 0, 0);
        
        pthread_create(&thread->thread_id, NULL, 
                      tx_thread_worker, (void*)thread);
    }

    return pool;
}

void tx_pool_destroy(tx_threads_pool_t* pool) {
    if (!pool) {
        return;
    }

    for (int i = 0; i < pool->threads_count; i++) {
        if (pool->threads[i].is_created) {
            pool->threads[i].end_flag = 1;
            sem_post(&pool->threads[i].task_sem);
        }
    }

    for (int i = 0; i < pool->threads_count; i++) {
        if (pool->threads[i].is_created) {
            pthread_join(pool->threads[i].thread_id, NULL);
            DEBUG_PRINTF("Thread %d joined\n", i);
        }
    }

    for (int i = 0; i < pool->threads_count; i++) {
        if (pool->threads[i].is_created) {
            sem_destroy(&pool->threads[i].task_sem);
        }
    }

    sem_destroy(&pool->available_threads);
    pthread_mutex_destroy(&pool->pool_mutex);

    free(pool);
    thread_pool = NULL;

    DEBUG_PRINTF("Thread pool destroyed\n");
}

void* tx_thread_worker(void* arg) {
    tx_thread_t* thread = (tx_thread_t*)arg;
    tx_threads_pool_t* pool = thread_pool; 

    while (1) {
        sem_wait(&thread->task_sem);

        if (thread->end_flag == 1) break;

        char filename[256];
        int client_tx_fd = thread->client_tx_fd;
        strcpy(filename, thread->filename);

        int result = send_file_to_client(client_tx_fd, filename);

        pthread_mutex_lock(&pool->pool_mutex);
        thread->is_working = 0;
        thread->client_id = -1;
        thread->client_tx_fd = -1;
        thread->tasks_completed++;
        pthread_mutex_unlock(&pool->pool_mutex);

        sem_post(&pool->available_threads);
        if (result != 0) {
            DEBUG_PRINTF("Thread %d: file transfer failed\n", thread->thread_num);
        }
    }

    DEBUG_PRINTF("Thread %d finished\n", thread->thread_num);
    return NULL;
}

int tx_pool_submit_task(tx_threads_pool_t* pool, int client_id,
                       int client_tx_fd, const char* filename) {
    if (sem_wait(&pool->available_threads) == -1) {
        perror("sem_wait");
        return -1;
    }

    pthread_mutex_lock(&pool->pool_mutex);

    tx_thread_t* free_thread = find_available_thread(pool);
    if (!free_thread) {
        pthread_mutex_unlock(&pool->pool_mutex);
        sem_post(&pool->available_threads);
        return -1;
    }

    free_thread->client_id = client_id;
    free_thread->client_tx_fd = client_tx_fd;
    strncpy(free_thread->filename, filename, sizeof(free_thread->filename) - 1);
    free_thread->filename[sizeof(free_thread->filename) - 1] = '\0';

    pthread_mutex_unlock(&pool->pool_mutex);

    sem_post(&free_thread->task_sem);

    return 0;
}

int find_free_thread_slot(tx_threads_pool_t* pool) {
    if (!pool) {
        DEBUG_PRINTF("ERROR: null ptr\n");
        return -1;
    }

    int amount = pool->threads_count;
    for (int i = 0; i < amount; ++i) {
        if (!pool->threads[i].is_created) {
            return i;
        }
    }

    DEBUG_PRINTF("Warning: threads are full\n");
    return -1;
}

tx_thread_t* find_available_thread(tx_threads_pool_t* pool) {
    if (!pool) {
        DEBUG_PRINTF("ERROR: null ptr\n");
        return NULL;
    }

    int amount = pool->threads_count;
    for (int i = 0; i < amount; i++) {
        if (pool->threads[i].is_created && !pool->threads[i].is_working) {
            pool->threads[i].is_working = 1;
            return pool->threads + i;
        }
    }

    DEBUG_PRINTF("Warning: all threads are busy\n");
    return NULL;
}

tx_thread_t* create_thread(tx_threads_pool_t* pool) {
    if (!pool) {
        DEBUG_PRINTF("ERROR: null ptr\n");
        return NULL;
    }

    int thread_num = find_free_thread_slot(pool);
    if (thread_num == -1) {
        return NULL;
    }

    tx_thread_t* thread = &pool->threads[thread_num];

    thread->thread_num = thread_num;
    thread->is_working = 0;
    thread->is_created = 1;
    thread->end_flag   = 0;
    thread->tasks_completed = 0;
    thread->client_id = -1;
    thread->client_tx_fd = -1;
    thread->filename[0] = '\0';

    if (sem_init(&thread->task_sem, 0, 0) != 0) {
        DEBUG_PRINTF("ERROR: sem_init failed\n");
        return NULL;
    }

    if (pthread_create(&thread->thread_id, NULL,
                       tx_thread_worker, (void*)thread) != 0) {
        DEBUG_PRINTF("ERROR: pthread_create failed\n");
        sem_destroy(&thread->task_sem);
        thread->is_created = 0;
        return NULL;
    }

    DEBUG_PRINTF("✓ Thread %d created dynamically\n", thread_num);
    return thread;
}
