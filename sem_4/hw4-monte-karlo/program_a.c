#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <semaphore.h>

#define SHM_NAME "/mс_shared_memory"
#define SEM_READY "/mc_data_ready"
#define SEM_RECEIVED "/mc_data_received"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct shared_memory {
    long total_hits;
    long total_points;
    double time;
    int n_threads;
};

struct thread_args {
    int thread_id;
    long points_per_thread;
    struct shared_memory *shm;
};

double f(double x) {
    return x;
}

void* worker_thread(void* arg) {
    struct thread_args* args = (struct thread_args*)arg;
    long points = args->points_per_thread;
    struct shared_memory *shm = args->shm;
    
    unsigned int seed = time(NULL) + args->thread_id;
    long local_hits = 0;
    
    for (long i = 0; i < points; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;
        if (y <= f(x)) local_hits++;
    }
    
    pthread_mutex_lock(&mutex);
    shm->total_hits += local_hits;
    pthread_mutex_unlock(&mutex);
    
    free(args);
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        perror ("Uncorrect arguments number");
        return 1;
    }
    
    int n_threads = atoi(argv[1]);
    long total_points = atol(argv[2]);
    
    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    sem_t *sem_received = sem_open(SEM_RECEIVED, O_CREAT, 0666, 0);
    
    if (sem_ready == SEM_FAILED || sem_received == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct shared_memory));
    struct shared_memory *shm = mmap(NULL, sizeof(struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    shm->total_hits = 0;
    shm->total_points = total_points;
    shm->n_threads = n_threads;
    
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    pthread_t threads[n_threads];
    long points_per_thread = total_points / n_threads;
    
    for (int i = 0; i < n_threads; i++) {
        struct thread_args* args = malloc(sizeof(struct thread_args));
        args->thread_id = i;
        args->points_per_thread = points_per_thread;
        args->shm = shm;
        pthread_create(&threads[i], NULL, worker_thread, args);
    }
    
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = (end_time.tv_sec - start_time.tv_sec) + 
                    (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    shm->time = elapsed;
    
    sem_post(sem_ready);
    if (sem_wait(sem_received) == -1) 
    {
        perror("sem_wait");
    }
    
    pthread_mutex_destroy(&mutex);
    munmap(shm, sizeof(struct shared_memory));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    
    sem_close(sem_ready);
    sem_close(sem_received);
    sem_unlink(SEM_READY);
    sem_unlink(SEM_RECEIVED);
    return 0;
}