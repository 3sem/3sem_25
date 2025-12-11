#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#define SHM_NAME "shm"
#define SEM_READY "ready"
#define SEM_DONE "done"

typedef struct
{
    long total_hits;
    long total_points;
    size_t threads_amt;
    double time;
    double a, b;
    double y_min, y_max;
} Shm;

typedef struct
{
    long hits;
    char padding[64 - sizeof(long)];
} ThrdRes;

typedef struct
{
    int thread_id;
    long points_per_thread;
    ThrdRes *results;
    Shm *shm;
} ThrdArgs;

double func(double x) 
{
    return x * x;
}


void* thread_process(void* arg) 
{
    ThrdArgs *args = (ThrdArgs*)arg;
    Shm *shm = args->shm;
    unsigned int seed = time(NULL) + args->thread_id;
    long local_hits = 0;

    for (long i = 0; i < args->points_per_thread; i++) 
    {
        double x = shm->a + (double)rand_r(&seed) / RAND_MAX * (shm->b - shm->a);
        double y = shm->y_min + (double)rand_r(&seed) / RAND_MAX * (shm->y_max - shm->y_min);
        if (y <= func(x))
            local_hits++;
    }

    args->results[args->thread_id].hits = local_hits;
    
    free(args);
    return NULL;
}

int main(int argc, char** argv) 
{
    size_t threads_amt = atoi(argv[1]);
    long total_points = atol(argv[2]);
    double a = atof(argv[3]);
    double b = atof(argv[4]);
    double max_f = atof(argv[5]);
    
    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    sem_t *sem_done = sem_open(SEM_DONE, O_CREAT, 0666, 0);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Shm));
    Shm *shm = mmap(NULL, sizeof(Shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    shm->total_hits = 0;
    shm->total_points = total_points;
    shm->threads_amt = threads_amt;
    shm->a = a;
    shm->b = b;
    shm->y_min = 0;
    shm->y_max = max_f;

    ThrdRes *results = malloc(threads_amt * sizeof(ThrdRes));
    for (size_t i = 0; i < threads_amt; i++)
        results[i].hits = 0;

    pthread_t *threads = malloc(threads_amt * sizeof(pthread_t));
    long points_per_thread = total_points / threads_amt;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (size_t i = 0; i < threads_amt; i++) {
        ThrdArgs *args = malloc(sizeof(ThrdArgs));
        args->thread_id = i;
        args->points_per_thread = points_per_thread;
        args->results = results;
        args->shm = shm;
        
        pthread_create(&threads[i], NULL, thread_process, args);
    }

    for (size_t i = 0; i < threads_amt; i++) 
        pthread_join(threads[i], NULL);

    for (size_t i = 0; i < threads_amt; i++) 
        shm->total_hits += results[i].hits;

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    shm->time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    sem_post(sem_ready);
    sem_wait(sem_done);

    free(threads);
    free(results);
    munmap(shm, sizeof(Shm));
    close(shm_fd);
    shm_unlink(SHM_NAME);
    sem_close(sem_ready);
    sem_unlink(SEM_READY);
    sem_close(sem_done);
    sem_unlink(SEM_DONE);

    return 0;
}