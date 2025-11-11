#include "common.h"
#include "mq_transfer.h"

void mq_copy_file (const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file) 
    {
        perror("Fopen failed");
        return;
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    key_t key = ftok(".", 'Q');
    if (key == -1) 
    {
        perror("ftok failed");
        fclose(file);
        return;
    }

    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) 
    {
        perror("Msgget failed");
        fclose(file);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Fork failed");
        fclose(file);
        msgctl(msgid, IPC_RMID, NULL);
        return;
    }
    else if (pid == 0) 
    {
        char output_filename[MAX_FILENAME];
        snprintf(output_filename, MAX_FILENAME, "copy_%s", filename);
        
        FILE *output_file = fopen(output_filename, "wb");
        if (!output_file) 
        {
            perror("Fopen failed");
            exit(1);
        }

        message_t msg;
        size_t total_received = 0;
        
        while (1) 
        {
            if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) 
            {
                perror("Msgrcv failed");
                break;
            }
            
            if (msg.msg_type == MSG_TYPE_END) 
                break;
            
            if (msg.bytes_used > 0) 
            {
                fwrite(msg.data, 1, msg.bytes_used, output_file);
                total_received += msg.bytes_used;
            }
        }
        
        fclose(output_file);
        exit(0);
    } else {
        printf("---------------------------------------\n");
        printf("Starting transfering...\n");
        size_t total_sent = 0;
        size_t chunks_total = (file_size + BUF_SIZE - 1) / BUF_SIZE;

        struct timeval start_time;
        start_timing(&start_time);

        message_t msg;
        
        for (size_t chunk_num = 0; chunk_num < chunks_total; chunk_num++) {
            size_t bytes_read = fread(msg.data, 1, BUF_SIZE, file);
            msg.bytes_used = bytes_read;
            msg.msg_type = MSG_TYPE_DATA;
            total_sent += bytes_read;

            if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) 
            {
                perror("Msgsnd failed");
                break;
            }
        }

        msg.msg_type = MSG_TYPE_END;
        msg.bytes_used = 0;
        msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0);

        printf("Transfering completed (%ld bytes).\n", total_sent);
        wait(NULL);

        double copy_time = stop_timing(&start_time);
        printf("Total time: %.2f seconds\n", copy_time);
        printf("---------------------------------------\n");
        
        fclose(file);
        msgctl(msgid, IPC_RMID, NULL);
    }
}