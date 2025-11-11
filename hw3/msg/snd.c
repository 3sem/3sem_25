#define _POSIX_C_SOURCE 199309L
#include "common.h"
#include <time.h>

int main() 
{
    struct message msg;
    int msgid = msgget(KEY, 0666 | IPC_CREAT);

    int fd = open(IN_FILE_NAME, O_RDONLY);

    char *buffer = (char*)calloc(BUF_SIZE, sizeof(char));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t bytes_read = 0;
    while ((bytes_read = read(fd, buffer, BUF_SIZE)) > 0) 
    {
        for (size_t offset = 0; offset < bytes_read; offset += MAX_MSG_SIZE) 
        {
            size_t chunk_size = (bytes_read - offset < MAX_MSG_SIZE) ? bytes_read - offset : MAX_MSG_SIZE;
        
            msg.mtype = MSG_DATA;
            msg.data_size = chunk_size;
            memcpy(msg.data, buffer + offset, chunk_size);
        
            msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);

            struct message ack_msg;
            msgrcv(msgid, &ack_msg, sizeof(ack_msg) - sizeof(long), MSG_ACK, 0);
        }
    }
    free(buffer);
    close(fd);

    struct message end_msg;
    end_msg.mtype = MSG_END;
    end_msg.data_size = 0;

    msgsnd(msgid, &end_msg, sizeof(end_msg) - sizeof(long), 0);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time: %.3f seconds\n", time_taken);
    
    return 0;
}