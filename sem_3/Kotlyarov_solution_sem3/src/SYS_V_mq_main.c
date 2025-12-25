#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <stdbool.h>
#include <time.h>
#include "Debug_printf.h"

#define MQ_SIZE (4096)

struct mqbuf {

    long mtype;
    char mtext[MQ_SIZE];
};

char EOF_MSG[] = "EOF";

int main(int argc, char *argv[]) {

    if (argc != 2) {

        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(1);
    }

    int fd_input_file = open(argv[1], O_RDONLY);
    if (fd_input_file == -1) {

        perror("open");
        return 1;
    }

    key_t eof_flag_key = ftok(".", 'F');
    if (eof_flag_key == -1) {

        perror("ftok (shm)");
        close(fd_input_file);
        return 1;
    }

    int shmid = shmget(eof_flag_key, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {

        perror("shmget");
        close(fd_input_file);
        return 1;
    }

    int* eof_flag_ptr = (int*) shmat(shmid, NULL, 0);
    if (eof_flag_ptr == (void*) -1) {

        perror("shmat");
        close(fd_input_file);
        return 1;
    }

    *eof_flag_ptr = 0;

    key_t mq_key = ftok(".", 'Q');
    if (mq_key == -1) {

        perror("ftok (shm)");
        close(fd_input_file);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    int mqid = msgget(mq_key, IPC_CREAT | 0660);
    if (mqid == -1) {

        perror("shmget");
        close(fd_input_file);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        return 1;
    }

    struct mqbuf msg = { .mtype = 1};
    struct timespec start, end;
    
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }

    if (pid == 0) {

        while (1) {

            ssize_t curr_size = msgrcv(mqid, &msg, MQ_SIZE, 0, 0);

            if(curr_size < 0) {

                perror("msgrcv");
                shmdt(eof_flag_ptr);
                exit(1);
            }

            else if(*eof_flag_ptr && !memcmp(msg.mtext, EOF_MSG, sizeof(EOF_MSG))) {

                break;
            }
                
            else {

                if(write(1, msg.mtext, curr_size) < 0) {

                    perror("write");
                    shmdt(eof_flag_ptr);
                    exit(1);
                }
            }
        }

        shmdt(eof_flag_ptr);
        exit(0);
    }

    else {

        int64_t curr_size = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while ((curr_size = read(fd_input_file, msg.mtext, MQ_SIZE)) > 0) {

            if(msgsnd(mqid, &msg, curr_size, 0)) {

                perror("msgsnd");
                break;
            }
        }

        *eof_flag_ptr = 1;
        memcpy(msg.mtext, EOF_MSG, sizeof(EOF_MSG));
        msgsnd(mqid, &msg, sizeof(EOF_MSG), 0);

        close(fd_input_file);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", argv[0], time_taken);
        shmdt(eof_flag_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        msgctl(mqid, IPC_RMID, NULL);
        if (curr_size == -1) {

            perror("read");
            return 1;
        }
    }

    return 0;
}