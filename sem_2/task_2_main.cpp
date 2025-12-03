#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "task_2_pipe.h"

const int buffer_size = 4096;
struct timespec start, end;

int main() {

    FILE* input_file  = fopen("test_file.txt", "r");
    FILE* output_file = fopen("output.txt", "w");

    Pipe duplex_pipe = {};
    pipe_ctor(&duplex_pipe);

    pid_t pid = fork();
    double elapsed = 0;


    if (pid == 0) {
        close(duplex_pipe.fd_direct[1]); // не пишет родителю
        close(duplex_pipe.fd_back[0]);   // не читает от себя
        child_process(&duplex_pipe);

    }
    else if (pid > 0) { // parent
        close(duplex_pipe.fd_direct[0]); // не читает от себя
        close(duplex_pipe.fd_back[1]);   // не пишет себе

        double overall_transfer_time = parent_process(&duplex_pipe, input_file, output_file);
        wait(NULL);

        printf("Pure data transfer time: %.6f s\n", overall_transfer_time);
    }
    else if (pid == -1) {
        perror("Error, can't fork\n");
    }
    pipe_dtor(&duplex_pipe);
    fclose(input_file);
    fclose(output_file);
    return 0;
}

void child_process(Pipe* duplex_pipe) {
    while (1) {
            if (duplex_pipe->actions.rcv(duplex_pipe, CHILD) <= 0) break;

            if (strcmp(duplex_pipe->data, "exit") == 0) {
                return;
            }

            duplex_pipe->actions.snd(duplex_pipe, CHILD);
            duplex_pipe->len = buffer_size;
        }
}

double parent_process(Pipe* duplex_pipe, FILE* input_file, FILE* output_file) {

    if (input_file == nullptr || output_file == nullptr) {
            perror("Can't open file\n");
            return -1.0;
    }

    int num = 0;
    double transfer_time = 0.0;

    while ((num = fread(duplex_pipe->data, sizeof(char), buffer_size, input_file))) {

        duplex_pipe->data[num] = '\0';

        if (num != buffer_size) {
            duplex_pipe->len = strlen(duplex_pipe->data);
        }

        clock_gettime(CLOCK_MONOTONIC, &start);
        int send = duplex_pipe->actions.snd(duplex_pipe, PARENT);

        while (!duplex_pipe->actions.rcv(duplex_pipe, PARENT)) {}
        clock_gettime(CLOCK_MONOTONIC, &end);
        transfer_time += (end.tv_sec - start.tv_sec) +
                               (end.tv_nsec - start.tv_nsec) / 1e9;

        fwrite(duplex_pipe->data, sizeof(char), duplex_pipe->len, output_file);
        duplex_pipe->len = buffer_size;
    }

    strcpy(duplex_pipe->data, "exit");
    duplex_pipe->actions.snd(duplex_pipe, PARENT);

    return transfer_time;
}

