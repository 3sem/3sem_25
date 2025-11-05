#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <time.h>
#include <stdbool.h>
#include "Debug_printf.h"

#define SHM_SIZE (4096 * 1024)

struct shared_data {
    char buffer[SHM_SIZE];
    size_t BUF_SIZE;
    char eof_flag;  // if 1 => eof
};

bool sem_w8_(int semid, int sem_num) { // sem_wAIT (wEIGHT)))), just to rename
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;  // wait untill > 0
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {

        perror("semop sem_w8_");
        return false;
    }

    return true;
}

bool sem_post_(int semid, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;  // incr sem
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {

        perror("semop sem_post_");
        return false;
    }

    return true;
}

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

    key_t shm_key = ftok(".", 'M');
    if (shm_key == -1) {
        perror("ftok (shm)");
        close(fd_input_file);
        return 1;
    }

    int shmid = shmget(shm_key, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        close(fd_input_file);
        return 1;
    }

    struct shared_data* shm_ptr = (struct shared_data*) shmat(shmid, NULL, 0);
    if (shm_ptr == (void*) -1) {
        perror("shmat");
        close(fd_input_file);
        return 1;
    }

    key_t sem_key = ftok(".", 'S'); 
    if (sem_key == -1) {
        perror("ftok (sem)");
        shmdt(shm_ptr);
        close(fd_input_file);
        return 1;
    }

    int semid = semget(sem_key, 2, IPC_CREAT | 0666); // [0] - buff is full, [1] - is free
    if (semid == -1) {
        perror("semget");
        shmdt(shm_ptr);
        close(fd_input_file);
        return 1;
    }

    if (semctl(semid, 0, SETVAL, 1) == -1) { // sem_empty = 1
        perror("semctl SETVAL 0");
        shmdt(shm_ptr);
        close(fd_input_file);
        return 1;
    }

    if (semctl(semid, 1, SETVAL, 0) == -1) { // sem_full = 0
        perror("semctl SETVAL 1");
        shmdt(shm_ptr);
        close(fd_input_file);
        return 1;
    }
    
    struct timespec start, end;

    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }

    if (pid == 0) {

        while (1) {

            if(!sem_w8_(semid, 1)) {

                shmdt(shm_ptr);
                exit(1);
            }

            if(shm_ptr->eof_flag) {

                if(!sem_post_(semid, 0)) {

                    shmdt(shm_ptr);
                    exit(1);
                }
                break;
            }
            
            size_t bytes_written_total = 0;
            size_t bytes_to_write = shm_ptr->BUF_SIZE;
            const char *buf_ptr = shm_ptr->buffer;

            while (bytes_written_total < bytes_to_write) {
                ssize_t curr_size = write(1, buf_ptr + bytes_written_total, bytes_to_write - bytes_written_total);

                if (curr_size == -1) {

                    perror("write");
                    shmdt(shm_ptr);
                    exit(1);
                }

                bytes_written_total += curr_size;
            }

            if(!sem_post_(semid, 0)) {

                shmdt(shm_ptr);
                exit(1);
            }
        }

        shmdt(shm_ptr);
        exit(0);
    }

    else {

        int64_t curr_size = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while (1) {
            
            if (!sem_w8_(semid, 0)) 
                break;

            curr_size = read(fd_input_file, shm_ptr->buffer, SHM_SIZE);

            if (curr_size > 0) {

                shm_ptr->BUF_SIZE = curr_size;
                sem_post_(semid, 1); 
            } 

            else if (curr_size == 0) {

                shm_ptr->eof_flag = 1;
                sem_post_(semid, 1);  
                break;
            } 
            
            else {

                sem_post_(semid, 0);  
                break;
            }
        }

        if (curr_size == 0) { 
            
            if (!sem_w8_(semid, 0)) {

                perror("sem_w8_ EOF wait empty");
            }
            
            else {

                shm_ptr->eof_flag = 1; 

                if (!sem_post_(semid, 1)) 
                    perror("sem_post_ EOF signal full");
            }
        } 

        else if (curr_size == -1) {

            perror("read");
        }

        close(fd_input_file);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", argv[0], time_taken);
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(semid, 0, IPC_RMID, NULL);
    }

    return 0;
}