#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>

struct msgbuf {
    long mtype;
    char mtext[]; 
};

void md5check (char* infile, char *outfile) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),"md5sum %s > %s", outfile, infile);
    system(cmd);
}

int main(int argc, char **argv){
    char *infile = argv[1];
    char *outfile = argv[2];
    int buf = atoi(argv[3]);

    int msq = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);

    pid_t pid = fork();
    struct timespec t0, t1;
    if (pid == 0) {
        // receiver
        FILE *out = fopen(outfile, "w");
        while (1) {
            struct msgbuf *m = malloc(sizeof(long) + buf);
            msgrcv(msq, m, buf, 1, 0);
            int len;
            memcpy(&len, m->mtext, sizeof(int));

            if (len == 0) {
                free(m);
                break; 
            }

            fwrite(m->mtext + sizeof(int), 1, len, out);
            free(m);
        }
        fclose(out);    

        md5check("/tmp/md5_recv.txt", outfile);
        
        exit(0);
    } else {
        // sender
        FILE *in = fopen(infile, "r");
        clock_gettime(CLOCK_MONOTONIC, &t0);
        while (1) {
            char *bufdata = malloc(buf);
            int n = fread(bufdata, 1, buf - sizeof(int), in);
            struct msgbuf *m = malloc(sizeof(long) + buf);
            m->mtype = 1;
            memcpy(m->mtext, &n, sizeof(int));

            if (n > 0) {
                memcpy(m->mtext + sizeof(int), bufdata, n);
            }

            msgsnd(msq, m, sizeof(int) + n, 0);
            free(m);
            free(bufdata);

            if (n == 0) {
                break;
            }
        }

        fclose(in);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double elapsed = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

        md5check("/tmp/md5_src.txt", infile);

        printf("MQ elapsed: %.6f\n", elapsed);
        system("echo 'src md5:'; cat /tmp/md5_src.txt");
        system("echo 'recv md5:'; cat /tmp/md5_recv.txt");
        msgctl(msq, IPC_RMID, NULL);
    }
    return 0;
}
