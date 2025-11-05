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

#define BUF_SIZE (4096)
#define FIFO_NAME "FIFO_transmission_pipe"

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

    char* buf = (char*) calloc(BUF_SIZE, sizeof(char));
    if (!buf) {

        perror("calloc");
        return 1;
    }

    if(mknod(FIFO_NAME, S_IFIFO | 0666, 0) == -1) {

        perror("mknod");
        return 1;
    }

    struct timespec start, end;
    pid_t pid = fork();
    if (pid < 0) {

        DEBUG_PRINTF("fork failed!\n");
        return -1;
    }

    if (pid == 0) {

        int ret_val = 0;
        int fd_fifo = open(FIFO_NAME, O_RDONLY);
        if (fd_fifo == -1) {

            perror("open");
            return -1;
        }

        ssize_t curr_size_read = 0;
        while ((curr_size_read = read(fd_fifo, buf, BUF_SIZE)) > 0) {

            ssize_t bytes_written_total = 0;
            while (bytes_written_total < curr_size_read) {

                ssize_t curr_size_write = write(1, buf + bytes_written_total, curr_size_read - bytes_written_total);
                if (curr_size_write == -1) {

                    perror("write");
                    ret_val = 1;
                    break;
                }

                bytes_written_total += curr_size_write;
            }
        }

        close(fd_fifo);
        free(buf);
        unlink(FIFO_NAME);
        exit(ret_val);
    }

    else {

        int ret_val = 0;
        int fd_fifo = open(FIFO_NAME, O_WRONLY);
        if (fd_fifo == -1) {

            perror("open");
            return -1;
        }
        
        ssize_t curr_size_read = 0;
        clock_gettime(CLOCK_MONOTONIC, &start);
        while ((curr_size_read = read(fd_input_file, buf, BUF_SIZE)) > 0) {

            ssize_t bytes_written_total = 0;
            while (bytes_written_total < curr_size_read) {

                ssize_t curr_size_write = write(fd_fifo, buf + bytes_written_total, curr_size_read - bytes_written_total);
                if (curr_size_write == -1) {

                    perror("write");
                    ret_val = 1;
                    break;
                }

                bytes_written_total += curr_size_write;
            }
        }

        close(fd_input_file);
        close(fd_fifo);
        wait(NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;
        fprintf(stderr, "%s: Time consumed = %.6f seconds\n", argv[0], time_taken);
        if (curr_size_read == -1) {

            perror("read");
            return 1;
        }

        free(buf);
        unlink(FIFO_NAME);
        return ret_val;
    }

    return 0;
}