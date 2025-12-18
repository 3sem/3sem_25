#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

typedef struct {
    size_t len;
    char data[];
} shm_layout;

void p(int semid, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    semop(semid, &sb, 1);
}

void v(int semid, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    semop(semid, &sb, 1);
}

int main(int argc, char *argv[]) {
    if (argc != 4) return 1;
    const char *src = argv[1];
    const char *dst = argv[2];
    size_t buf_size = atol(argv[3]);

    int shmid = shmget(IPC_PRIVATE, sizeof(size_t) + buf_size, IPC_CREAT | 0666);
    int semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    
    unsigned short init_vals[2] = {1, 0};
    semctl(semid, 0, SETALL, init_vals);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();

    if (pid == 0) {
        shm_layout *mem = (shm_layout *)shmat(shmid, NULL, 0);
        FILE *fout = fopen(dst, "wb");
        
        while (1) {
            p(semid, 1);
            if (mem->len == 0) break;
            fwrite(mem->data, 1, mem->len, fout);
            v(semid, 0);
        }
        
        fclose(fout);
        shmdt(mem);
        exit(0);
    } else {
        shm_layout *mem = (shm_layout *)shmat(shmid, NULL, 0);
        FILE *fin = fopen(src, "rb");
        
        size_t n;
        while ((n = fread(mem->data, 1, buf_size, fin)) > 0) {
            p(semid, 0);
            mem->len = n;
            v(semid, 1);
        }
        
        p(semid, 0);
        mem->len = 0;
        v(semid, 1);

        fclose(fin);
        wait(NULL);
        shmdt(mem);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("SHM Time: %.4f s\n", seconds);

    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
