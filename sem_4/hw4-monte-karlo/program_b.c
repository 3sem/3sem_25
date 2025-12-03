#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

#define SHM_NAME "/mс_shared_memory"
#define SEM_READY "/mc_data_ready"
#define SEM_RECEIVED "/mc_data_received"

struct shared_memory {
    long total_hits;
    long total_points;
    double time;
    int n_threads;
};

int main() 
{
    sem_t *sem_ready = sem_open(SEM_READY, 0);
    sem_t *sem_received = sem_open(SEM_RECEIVED, 0);
    
    if (sem_ready == SEM_FAILED || sem_received == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    
    struct shared_memory *shm = mmap(NULL, sizeof(struct shared_memory), PROT_READ, MAP_SHARED, shm_fd, 0);
    
    if (sem_wait(sem_ready) == -1) {
        perror("sem_wait");
        return 1;
    }
    
    double integral = (double)shm->total_hits / shm->total_points;
    double expected = 1.0/2.0;
    
    printf("RESULTS OF INTEGRATION\n");
    printf("Function:  f(x) = x on [0, 1]\n");
    printf("Result:    ∫xdx ≈ %f\n", integral);
    printf("Expected:  ∫xdx = %f\n", expected);
    printf("Threads_n: %d\n", shm->n_threads);
    printf("All time:  %f\n", shm->time);
    
    sem_post(sem_received);
    
    munmap(shm, sizeof(struct shared_memory));
    close(shm_fd);
    sem_close(sem_ready);
    sem_close(sem_received);
    return 0;
}