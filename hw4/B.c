#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
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

int main() 
{
    sem_t *sem_ready = sem_open(SEM_READY, O_CREAT, 0666, 0);
    sem_t *sem_done = sem_open(SEM_DONE, O_CREAT, 0666, 0);
    
    sem_wait(sem_ready);

    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    Shm *shm = mmap(NULL, sizeof(Shm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    double area_rectangle = (shm->b - shm->a) * (shm->y_max - shm->y_min);
    double integral = ((double)shm->total_hits / shm->total_points) * area_rectangle;

    printf("threads amt: %ld\n", shm->threads_amt);
    printf("int val: %.4f\n", integral);
    printf("time: %.4f sec\n", shm->time);
    
    sem_post(sem_done); 

    munmap(shm, sizeof(Shm));
    close(shm_fd);
    sem_close(sem_ready);
    sem_close(sem_done);

    return 0;
}