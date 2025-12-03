#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shm_task.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_file>\n", argv[0]);
        exit(1);
    }

    key_t key = 1234;
    int shmid = shmget(key, sizeof(struct shmseg), 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    struct shmseg *shm_ptr = (struct shmseg *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "wb");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    while (!shm_ptr->ended || shm_ptr->ready) {
        if (shm_ptr->ready) {
            fwrite(shm_ptr->buf, 1, shm_ptr->bytes, fp);
            shm_ptr->ready = 0;
        } else {
            usleep(1000);
        }
    }

    fclose(fp);

    double total_time = now() - shm_ptr->start_time;
    printf("Reader: файл получен, время передачи = %.6f секунд\n", total_time);

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}

