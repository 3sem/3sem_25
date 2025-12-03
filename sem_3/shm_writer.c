#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shm_task.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(1);
    }

    key_t key = 1234;
    int shmid = shmget(key, sizeof(struct shmseg), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    struct shmseg *shm_ptr = (struct shmseg *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    shm_ptr->ready = 0;
    shm_ptr->ended = 0;
    shm_ptr->start_time = now();

    while (!feof(fp)) {
        size_t n = fread(shm_ptr->buf, 1, BUF_SIZE, fp);
        if (n == 0) break;

        shm_ptr->bytes = n;
        shm_ptr->ready = 1;

        while (shm_ptr->ready == 1)
            usleep(1000);
    }

    shm_ptr->ended = 1;

    fclose(fp);
    printf("Writer: данные отправлены.\n");

    shmdt(shm_ptr);
    return 0;
}

