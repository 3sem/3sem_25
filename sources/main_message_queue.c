#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>

#include "message_queue_send.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        perror("wrong file configuration for message queue test\n");
        return -1;
    }
    char *src_file = argv[1];
    char *dst_file = argv[2];

    // printf("started from file %s to file%s\n", src_file, dst_file);

    Mqueue *mqueue = ctor_mqueue();
    // printf("buf %p fd %d name %s size%d\n", mqueue->buffer, mqueue->fd, mqueue->name, mqueue->size);

    // printf("mqueue created and opened\n");

    struct timeval time_start, time_end, prog_time;

    gettimeofday(&time_start, NULL);

    send_file_mqueue(mqueue, src_file, dst_file);

    gettimeofday(&time_end, NULL);

    dtor_mqueue(mqueue);


    timersub(&time_end, &time_start, &prog_time);
    printf(
        "\x1b[32m""Time used to send file "
        "\x1b[34m""%s"
        "\x1b[32m"" with"
        "\x1b[35m"" message queue"
        "\x1b[32m"": %ld s %ld us\n\n"
        "\x1b[0m",
        src_file, (uint64_t)prog_time.tv_sec, (uint64_t)prog_time.tv_usec
    );
    
}
