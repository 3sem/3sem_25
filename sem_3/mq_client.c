#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "mq_task.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    mqd_t qd_server;
    if ((qd_server = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror("Client: mq_open");
        exit(1);
    }

    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror("Client: fopen");
        exit(1);
    }

    char buffer[MAX_MSG_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, MAX_MSG_SIZE, in)) > 0) {
        if (mq_send(qd_server, buffer, bytes_read, 0) == -1) {
            perror("Client: mq_send");
            exit(1);
        }
    }

    mq_send(qd_server, "EOF", 4, 0);

    fclose(in);
    mq_close(qd_server);

    printf("Client: file sent successfully.\n");
    return 0;
}

