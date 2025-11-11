#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

typedef struct {
    int id, n, N;
    int *shared;
} Args;

void* worker(void *arg) {
    Args *a = (Args*)arg;
    unsigned int seed = time(NULL) ^ (a->id * 7919);
    int count = 0;
    for (int i = 0; i < a->N / a->n; i++) {
        double x = rand_r(&seed) / (double)RAND_MAX;
        double y = rand_r(&seed) / (double)RAND_MAX;
        if (a->id < a->n / 2) {
            if (y <= x) count++;
        } else {
            if (y > x) count++;
        }
    }
    a->shared[a->id] = count;
    return NULL;
}

int main(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    int N = atoi(argv[2]);

    int fd = shm_open("/mc_shared", O_CREAT | O_RDWR, 0666);
    ftruncate(fd, n * sizeof(int));
    int *shared = mmap(NULL, n * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    pthread_t threads[n];
    Args args[n];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < n; i++) {
        args[i] = (Args){i, n, N, shared};
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }
    for (int i = 0; i < n; i++) pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("ProgA finished. Results written to shared memory.\n");
    printf("Elapsed time: %.3f s\n", elapsed);
    printf("Run progB to read results.\n");

    //run progB 
    char n_str[32], N_str[32];
    snprintf(n_str, sizeof(n_str), "%d", n);
    snprintf(N_str, sizeof(N_str), "%d", N);
    execl("./progB", "progB", n_str, N_str, (char *)NULL);

    munmap(shared, n * sizeof(int));
    close(fd);
    shm_unlink("/mc_shared");
    return 0;
}
