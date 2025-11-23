#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "mq_task.h"

int main() {
    mqd_t qd_server;
    struct mq_attr attr = {0, MAX_MESSAGES, MAX_MSG_SIZE, 0};

    if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Server: mq_open");
        exit(1);
    }

    printf("Server: waiting for file...\n");
    fflush(stdout);
    FILE *out = fopen("mq_received.txt", "wb");
    if (!out) {
        perror("Server: fopen");
        exit(1);
    }

    char buffer[MSG_BUFFER_SIZE];
    ssize_t bytes_read;
    struct timespec start, end;
    int first_chunk = 1;

    while (1) {
        bytes_read = mq_receive(qd_server, buffer, MSG_BUFFER_SIZE, NULL);
        if (bytes_read == -1) {
            perror("Server: mq_receive");
            exit(1);
        }

        if (first_chunk) {
            clock_gettime(CLOCK_MONOTONIC, &start);
            first_chunk = 0;
        }


        if  (strncmp(buffer, "EOF", 3) == 0 || strncmp(buffer, "EOF\n", 4) == 0)  {
            clock_gettime(CLOCK_MONOTONIC, &end);
            break;
        }

        fwrite(buffer, 1, bytes_read, out);
    }
    printf("Server: exiting receive loop\n");
    fflush(stdout);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    fclose(out);
    mq_close(qd_server);
    mq_unlink(SERVER_QUEUE_NAME);

    printf("Server: file received in %.6f seconds\n", elapsed);
    fflush(stdout);
    return 0;
}

