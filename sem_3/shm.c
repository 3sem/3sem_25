#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <pthread.h>

struct shm_block {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int len; 
    int ready;
    char data[]; 
};

void md5check(const char* outfile, const char* target) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "md5sum %s > %s", target, outfile);
    system(cmd);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s infile outfile bufsize\n", argv[0]);
        return 1;
    }

    char *infile = argv[1];
    char *outfile = argv[2];
    int buf = atoi(argv[3]);

    int shmid = shmget(IPC_PRIVATE, sizeof(struct shm_block) + buf, 0600 | IPC_CREAT);
    struct shm_block *blk = (struct shm_block*) shmat(shmid, NULL, 0);

    // initialize shared objects before fork
    pthread_mutexattr_t mtx_attr;
    pthread_condattr_t  cond_attr;
    pthread_mutexattr_init(&mtx_attr);
    pthread_condattr_init(&cond_attr);
    pthread_mutexattr_setpshared(&mtx_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&blk->mutex, &mtx_attr);
    pthread_cond_init(&blk->cond, &cond_attr);
    blk->len = 0;
    blk->ready = 0;

    pid_t pid = fork();
    struct timespec t0, t1;

    if (pid == 0) {
        // receiver
        FILE *out = fopen(outfile, "wb");
        while (1) {
            pthread_mutex_lock(&blk->mutex);
            while (blk->ready == 0) {
                pthread_cond_wait(&blk->cond, &blk->mutex);
            }

            int len = blk->len;
            if (len == 0) {
                pthread_mutex_unlock(&blk->mutex);
                break;
            }

            fwrite(blk->data, 1, len, out);
            blk->ready = 0;
            pthread_cond_signal(&blk->cond);
            pthread_mutex_unlock(&blk->mutex);
        }
        fclose(out);
        md5check("/tmp/md5_recv.txt", outfile);
        _exit(0);
    } else {
        // sender
        FILE *in = fopen(infile, "rb");
        clock_gettime(CLOCK_MONOTONIC, &t0);

        while (1) {
            pthread_mutex_lock(&blk->mutex);
            while (blk->ready == 1) {
                pthread_cond_wait(&blk->cond, &blk->mutex);
            }

            int n = fread(blk->data, 1, buf, in);
            blk->len = n;
            blk->ready = 1;
            pthread_cond_signal(&blk->cond);
            pthread_mutex_unlock(&blk->mutex);

            if (n == 0) break;
        }

        fclose(in);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed = (t1.tv_sec - t0.tv_sec) +
                         (t1.tv_nsec - t0.tv_nsec) / 1e9;

        md5check("/tmp/md5_src.txt", infile);
        printf("SHM elapsed: %.6f\n", elapsed);
        system("echo 'src md5:'; cat /tmp/md5_src.txt");
        system("echo 'recv md5:'; cat /tmp/md5_recv.txt");

        pthread_mutex_destroy(&blk->mutex);
        pthread_cond_destroy(&blk->cond);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}
