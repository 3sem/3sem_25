#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

struct msg_buf {
    long mtype;
    char mtext[];
};

int main(int argc, char *argv[]) {
    if (argc != 4) return 1;
    const char *src = argv[1];
    const char *dst = argv[2];
    size_t buf_size = atol(argv[3]);

    int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    
    struct msg_buf *msg = malloc(sizeof(long) + buf_size);
    if (!msg) return 1;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();

    if (pid == 0) {
        FILE *fout = fopen(dst, "wb");
        while (1) {
            ssize_t rc = msgrcv(msgid, msg, buf_size, 1, 0);
            if (rc < 0) break;
            if (rc == 0) break; 
            fwrite(msg->mtext, 1, rc, fout);
        }
        fclose(fout);
        exit(0);
    } else {
        FILE *fin = fopen(src, "rb");
        size_t n;
        msg->mtype = 1;
        
        while ((n = fread(msg->mtext, 1, buf_size, fin)) > 0) {
            if (msgsnd(msgid, msg, n, 0) == -1) {
                perror("msgsnd failed (buffer too big?)");
                kill(pid, SIGKILL);
                break;
            }
        }
        
        msgsnd(msgid, msg, 0, 0);

        fclose(fin);
        wait(NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double seconds = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("MQ Time: %.4f s\n", seconds);

    msgctl(msgid, IPC_RMID, NULL);
    free(msg);

    return 0;
}
